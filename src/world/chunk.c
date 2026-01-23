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

void chunk_update_active_state(Chunk* chunk) {
    u32 count = 0;
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
        for (i32 x = 0; x < CHUNK_SIZE; x++) {
            if (chunk->pixels[y][x].material_id != MAT_AIR) {
                count++;
            }
        }
    }
    chunk->active_count = count;
    chunk->active = count > 0;
}

bool chunk_is_sleeping(const Chunk* chunk) {
    return chunk->sleep_timer >= CHUNK_SLEEP_THRESHOLD;
}

void chunk_wake(Chunk* chunk) {
    chunk->sleep_timer = 0;
}

void chunk_try_sleep(Chunk* chunk) {
    if (!chunk->dirty && chunk->active_count == 0) {
        chunk->sleep_timer++;
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
