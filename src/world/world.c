#include "world/world.h"
#include "world/chunk.h"
#include "physics/physics.h"
#include "core/config.h"
#include "core/thread_pool.h"
#include <stdlib.h>
#include <stdio.h>

void world_init(World* world) {
    world->tick = 0;
    world->ambient_temperature = AMBIENT_TEMPERATURE;
    world->gravity = GRAVITY;
    world->paused = false;
    world->use_parallel = true;
    
    // Initialize thread pool
    i32 num_threads = thread_pool_get_num_cores();
    // Use n-1 threads to leave one for main thread
    if (num_threads > 1) num_threads--;
    
    world->thread_pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (world->thread_pool) {
        if (!thread_pool_init(world->thread_pool, num_threads)) {
            fprintf(stderr, "Warning: Failed to init thread pool, using single-threaded mode\n");
            free(world->thread_pool);
            world->thread_pool = NULL;
            world->use_parallel = false;
        }
    } else {
        world->use_parallel = false;
    }
    
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            chunk_init(&world->chunks[cy][cx], cx, cy);
        }
    }
}

void world_destroy(World* world) {
    if (world->thread_pool) {
        thread_pool_shutdown(world->thread_pool);
        free(world->thread_pool);
        world->thread_pool = NULL;
    }
}

void world_clear(World* world) {
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            chunk_clear(&world->chunks[cy][cx]);
        }
    }
    world->tick = 0;
}

void world_reset_update_flags(World* world) {
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            chunk_reset_update_flags(&world->chunks[cy][cx]);
        }
    }
}

void world_update(World* world) {
    if (world->paused) return;
    
    world_reset_update_flags(world);
    physics_update(world, FIXED_TIMESTEP);
    world->tick++;
}

void world_toggle_pause(World* world) {
    world->paused = !world->paused;
}

Pixel* world_get_pixel(World* world, i32 x, i32 y) {
    if (!world_in_bounds(x, y)) return NULL;
    
    i32 chunk_x = x / CHUNK_SIZE;
    i32 chunk_y = y / CHUNK_SIZE;
    i32 local_x = x % CHUNK_SIZE;
    i32 local_y = y % CHUNK_SIZE;
    
    return chunk_get_pixel(&world->chunks[chunk_y][chunk_x], local_x, local_y);
}

bool world_set_pixel(World* world, i32 x, i32 y, u16 material_id) {
    return world_set_pixel_with_temp(world, x, y, material_id, world->ambient_temperature);
}

bool world_set_pixel_with_temp(World* world, i32 x, i32 y, u16 material_id, f32 temp) {
    if (!world_in_bounds(x, y)) return false;
    
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    pixel_init_with_temp(pixel, material_id, temp);
    
    Chunk* chunk = world_get_chunk_at_pixel(world, x, y);
    if (chunk) {
        chunk_wake(chunk);
        chunk_mark_changed(chunk);
        chunk->needs_rebuild = true;
    }
    
    return true;
}

void world_swap_pixels(World* world, i32 x1, i32 y1, i32 x2, i32 y2) {
    if (!world_in_bounds(x1, y1) || !world_in_bounds(x2, y2)) return;
    
    Pixel* p1 = world_get_pixel(world, x1, y1);
    Pixel* p2 = world_get_pixel(world, x2, y2);
    
    if (!p1 || !p2) return;
    
    Pixel temp = *p1;
    *p1 = *p2;
    *p2 = temp;
    
    // Wake and mark changed for proper sleep tracking
    Chunk* c1 = world_get_chunk_at_pixel(world, x1, y1);
    Chunk* c2 = world_get_chunk_at_pixel(world, x2, y2);
    
    if (c1) {
        chunk_wake(c1);
        chunk_mark_changed(c1);
        c1->needs_rebuild = true;
    }
    if (c2 && c2 != c1) {
        chunk_wake(c2);
        chunk_mark_changed(c2);
        c2->needs_rebuild = true;
    }
}

bool world_move_pixel(World* world, i32 from_x, i32 from_y, i32 to_x, i32 to_y) {
    if (!world_in_bounds(from_x, from_y) || !world_in_bounds(to_x, to_y)) return false;
    
    Pixel* from = world_get_pixel(world, from_x, from_y);
    Pixel* to = world_get_pixel(world, to_x, to_y);
    
    if (!from || !to) return false;
    if (to->material_id != MAT_AIR) return false;
    
    *to = *from;
    pixel_reset(from);
    
    // Wake and mark changed for proper sleep tracking
    Chunk* c_from = world_get_chunk_at_pixel(world, from_x, from_y);
    Chunk* c_to = world_get_chunk_at_pixel(world, to_x, to_y);
    
    if (c_from) {
        chunk_wake(c_from);
        chunk_mark_changed(c_from);
        c_from->needs_rebuild = true;
    }
    if (c_to && c_to != c_from) {
        chunk_wake(c_to);
        chunk_mark_changed(c_to);
        c_to->needs_rebuild = true;
    }
    
    return true;
}

Chunk* world_get_chunk(World* world, i32 chunk_x, i32 chunk_y) {
    if (chunk_x < 0 || chunk_x >= CHUNKS_X || chunk_y < 0 || chunk_y >= CHUNKS_Y) return NULL;
    return &world->chunks[chunk_y][chunk_x];
}

Chunk* world_get_chunk_at_pixel(World* world, i32 x, i32 y) {
    if (!world_in_bounds(x, y)) return NULL;
    return world_get_chunk(world, x / CHUNK_SIZE, y / CHUNK_SIZE);
}

void world_wake_chunk_at(World* world, i32 x, i32 y) {
    Chunk* chunk = world_get_chunk_at_pixel(world, x, y);
    if (chunk) {
        chunk_wake(chunk);
        chunk_mark_dirty(chunk);
    }
}

bool world_in_bounds(i32 x, i32 y) {
    return x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT;
}

void world_to_chunk_coords(i32 world_x, i32 world_y, i32* chunk_x, i32* chunk_y, i32* local_x, i32* local_y) {
    *chunk_x = world_x / CHUNK_SIZE;
    *chunk_y = world_y / CHUNK_SIZE;
    *local_x = world_x % CHUNK_SIZE;
    *local_y = world_y % CHUNK_SIZE;
}
