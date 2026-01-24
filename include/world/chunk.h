#ifndef PIXELSIM_CHUNK_H
#define PIXELSIM_CHUNK_H

#include "core/types.h"
#include "core/config.h"
#include "world/pixel.h"

// Maximum active pixels per chunk for the active list
#define MAX_ACTIVE_PER_CHUNK (CHUNK_SIZE * CHUNK_SIZE)

// Packed coordinate: (y << 6) | x for 64x64 chunk
typedef u16 PackedCoord;

#define PACK_COORD(x, y) ((PackedCoord)(((y) << 6) | (x)))
#define UNPACK_X(c) ((i32)((c) & 0x3F))
#define UNPACK_Y(c) ((i32)((c) >> 6))

// Chunk - 64x64 region of pixels
typedef struct Chunk {
    Pixel pixels[CHUNK_SIZE][CHUNK_SIZE];
    
    // Active pixel tracking for O(active) instead of O(n^2) iteration
    PackedCoord active_list[MAX_ACTIVE_PER_CHUNK];
    u16 active_count;            // Number of non-air pixels
    
    // Movement tracking - pixels that moved this tick
    bool needs_rebuild;          // Active list needs rebuilding
    
    bool dirty;                  // Needs re-render
    bool active;                 // Has any activity
    i32 sleep_timer;             // Ticks since last activity
    i32 chunk_x;                 // Chunk position in world
    i32 chunk_y;
    
    // Statistics for adaptive sleep
    u32 changes_this_tick;       // How many pixels changed
    u32 last_tick_changes;       // Changes in previous tick
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

// Active pixel list management
void   chunk_rebuild_active_list(Chunk* chunk);
void   chunk_add_active(Chunk* chunk, i32 local_x, i32 local_y);
void   chunk_remove_active(Chunk* chunk, i32 local_x, i32 local_y);
void   chunk_mark_changed(Chunk* chunk);

// Pixel access within chunk
Pixel* chunk_get_pixel(Chunk* chunk, i32 local_x, i32 local_y);
void   chunk_set_pixel(Chunk* chunk, i32 local_x, i32 local_y, const Pixel* pixel);
void   chunk_swap_pixels(Chunk* chunk, i32 x1, i32 y1, i32 x2, i32 y2);

// Fast inline pixel access (no bounds check - use carefully)
static inline Pixel* chunk_get_pixel_fast(Chunk* chunk, i32 local_x, i32 local_y) {
    return &chunk->pixels[local_y][local_x];
}

// Bounds checking
bool   chunk_local_in_bounds(i32 local_x, i32 local_y);

// Edge detection for neighbor chunk waking (inline for performance)
static inline bool chunk_has_pixel_at_bottom_edge(Chunk* chunk) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
        if (chunk->pixels[CHUNK_SIZE - 1][x].material_id != MAT_AIR) return true;
    }
    return false;
}

static inline bool chunk_has_pixel_at_top_edge(Chunk* chunk) {
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
        if (chunk->pixels[0][x].material_id != MAT_AIR) return true;
    }
    return false;
}

static inline bool chunk_has_pixel_at_left_edge(Chunk* chunk) {
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
        if (chunk->pixels[y][0].material_id != MAT_AIR) return true;
    }
    return false;
}

static inline bool chunk_has_pixel_at_right_edge(Chunk* chunk) {
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
        if (chunk->pixels[y][CHUNK_SIZE - 1].material_id != MAT_AIR) return true;
    }
    return false;
}

#endif
