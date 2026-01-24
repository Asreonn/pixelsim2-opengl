#include "world/chunk.h"
#include "materials/material_ids.h"
#include <string.h>

void chunk_init(Chunk* chunk, i32 chunk_x, i32 chunk_y) {
    chunk->chunk_x = chunk_x;
    chunk->chunk_y = chunk_y;
    chunk->dirty = true;
    chunk->active = false;
    chunk->active_count = 0;
    chunk->sleep_timer = 0;
    chunk->needs_rebuild = false;
    chunk->changes_this_tick = 0;
    chunk->last_tick_changes = 0;
    chunk_clear(chunk);
}

void chunk_clear(Chunk* chunk) {
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
        for (i32 x = 0; x < CHUNK_SIZE; x++) {
            chunk->pixels[y][x].material_id = MAT_AIR;
            chunk->pixels[y][x].temperature = AMBIENT_TEMPERATURE;
            chunk->pixels[y][x].velocity_x = 0;
            chunk->pixels[y][x].velocity_y = 0;
            chunk->pixels[y][x].pressure = AMBIENT_PRESSURE;
            chunk->pixels[y][x].lifetime = 0;
            chunk->pixels[y][x].updated = 0;
            chunk->pixels[y][x].color_seed = 0;
            chunk->pixels[y][x].flags = 0;
        }
    }
    chunk->dirty = true;
    chunk->active = false;
    chunk->active_count = 0;
    chunk->needs_rebuild = false;
    chunk->changes_this_tick = 0;
}

void chunk_reset_update_flags(Chunk* chunk) {
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
        for (i32 x = 0; x < CHUNK_SIZE; x++) {
            chunk->pixels[y][x].updated = 0;
        }
    }
}

void chunk_mark_dirty(Chunk* chunk) {
    chunk->dirty = true;
}

void chunk_rebuild_active_list(Chunk* chunk) {
    chunk->active_count = 0;
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
        for (i32 x = 0; x < CHUNK_SIZE; x++) {
            if (chunk->pixels[y][x].material_id != MAT_AIR) {
                chunk->active_list[chunk->active_count++] = PACK_COORD(x, y);
            }
        }
    }
    chunk->needs_rebuild = false;
}

void chunk_add_active(Chunk* chunk, i32 local_x, i32 local_y) {
    if (chunk->active_count < MAX_ACTIVE_PER_CHUNK) {
        chunk->active_list[chunk->active_count++] = PACK_COORD(local_x, local_y);
    }
    chunk_mark_changed(chunk);
}

void chunk_remove_active(Chunk* chunk, i32 local_x, i32 local_y) {
    // Mark for rebuild rather than searching the list
    chunk->needs_rebuild = true;
    chunk_mark_changed(chunk);
}

void chunk_mark_changed(Chunk* chunk) {
    chunk->changes_this_tick++;
    chunk->dirty = true;
}

void chunk_update_active_state(Chunk* chunk) {
    // Store last tick's changes for adaptive sleep
    chunk->last_tick_changes = chunk->changes_this_tick;
    chunk->changes_this_tick = 0;
    
    // Rebuild active list if needed
    if (chunk->needs_rebuild) {
        chunk_rebuild_active_list(chunk);
    }
    
    chunk->active = chunk->active_count > 0;
}

bool chunk_is_sleeping(const Chunk* chunk) {
    return chunk->sleep_timer >= CHUNK_SLEEP_THRESHOLD;
}

void chunk_wake(Chunk* chunk) {
    chunk->sleep_timer = 0;
}

void chunk_try_sleep(Chunk* chunk) {
    // Adaptive sleep: sleep if no changes in multiple consecutive ticks
    // Must have no activity for several frames before sleeping
    if (chunk->last_tick_changes == 0 && chunk->changes_this_tick == 0) {
        // Only increment if we have pixels (empty chunks can sleep faster)
        if (chunk->active_count > 0) {
            chunk->sleep_timer++;
        } else {
            chunk->sleep_timer += 2;  // Empty chunks sleep faster
        }
    } else {
        chunk->sleep_timer = 0;
    }
}

Pixel* chunk_get_pixel(Chunk* chunk, i32 local_x, i32 local_y) {
    if (!chunk_local_in_bounds(local_x, local_y)) return NULL;
    return &chunk->pixels[local_y][local_x];
}

void chunk_set_pixel(Chunk* chunk, i32 local_x, i32 local_y, const Pixel* pixel) {
    if (!chunk_local_in_bounds(local_x, local_y)) return;
    chunk->pixels[local_y][local_x] = *pixel;
    chunk->dirty = true;
    chunk_wake(chunk);
}

void chunk_swap_pixels(Chunk* chunk, i32 x1, i32 y1, i32 x2, i32 y2) {
    if (!chunk_local_in_bounds(x1, y1) || !chunk_local_in_bounds(x2, y2)) return;
    Pixel temp = chunk->pixels[y1][x1];
    chunk->pixels[y1][x1] = chunk->pixels[y2][x2];
    chunk->pixels[y2][x2] = temp;
    chunk->dirty = true;
}

bool chunk_local_in_bounds(i32 local_x, i32 local_y) {
    return local_x >= 0 && local_x < CHUNK_SIZE && 
           local_y >= 0 && local_y < CHUNK_SIZE;
}
