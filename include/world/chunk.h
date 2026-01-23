#ifndef PIXELSIM_CHUNK_H
#define PIXELSIM_CHUNK_H

#include "core/types.h"
#include "core/config.h"
#include "world/pixel.h"

// Chunk - 64x64 region of pixels
typedef struct Chunk {
    Pixel pixels[CHUNK_SIZE][CHUNK_SIZE];
    bool dirty;                  // Needs re-render
    bool active;                 // Has active pixels
    u32 active_count;            // Number of active pixels
    i32 sleep_timer;             // Ticks since last activity
    i32 chunk_x;                 // Chunk position in world
    i32 chunk_y;
} Chunk;

// Chunk functions
void   chunk_init(Chunk* chunk, i32 chunk_x, i32 chunk_y);
void   chunk_clear(Chunk* chunk);
void   chunk_reset_update_flags(Chunk* chunk);
void   chunk_mark_dirty(Chunk* chunk);
void   chunk_update_active_state(Chunk* chunk);
bool   chunk_is_sleeping(const Chunk* chunk);
void   chunk_wake(Chunk* chunk);
void   chunk_try_sleep(Chunk* chunk);

// Pixel access within chunk
Pixel* chunk_get_pixel(Chunk* chunk, i32 local_x, i32 local_y);
void   chunk_set_pixel(Chunk* chunk, i32 local_x, i32 local_y, const Pixel* pixel);
void   chunk_swap_pixels(Chunk* chunk, i32 x1, i32 y1, i32 x2, i32 y2);

// Bounds checking
bool   chunk_local_in_bounds(i32 local_x, i32 local_y);

#endif
