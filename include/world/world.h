#ifndef PIXELSIM_WORLD_H
#define PIXELSIM_WORLD_H

#include "core/types.h"
#include "core/config.h"
#include "world/chunk.h"

// World structure - contains all chunks
typedef struct World {
    Chunk chunks[CHUNKS_Y][CHUNKS_X];
    u64 tick;                    // Simulation tick counter
    f32 ambient_temperature;     // Default temperature (Kelvin)
    f32 gravity;                 // Gravity acceleration
    bool paused;                 // Simulation paused?
} World;

// World lifecycle
void   world_init(World* world);
void   world_destroy(World* world);
void   world_clear(World* world);
void   world_reset_update_flags(World* world);

// Simulation
void   world_update(World* world);
void   world_toggle_pause(World* world);

// Pixel access (world coordinates)
Pixel* world_get_pixel(World* world, i32 x, i32 y);
bool   world_set_pixel(World* world, i32 x, i32 y, u16 material_id);
bool   world_set_pixel_with_temp(World* world, i32 x, i32 y, u16 material_id, f32 temp);
void   world_swap_pixels(World* world, i32 x1, i32 y1, i32 x2, i32 y2);
bool   world_move_pixel(World* world, i32 from_x, i32 from_y, i32 to_x, i32 to_y);

// Chunk access
Chunk* world_get_chunk(World* world, i32 chunk_x, i32 chunk_y);
Chunk* world_get_chunk_at_pixel(World* world, i32 x, i32 y);
void   world_wake_chunk_at(World* world, i32 x, i32 y);

// Bounds checking
bool   world_in_bounds(i32 x, i32 y);

// Coordinate conversion
void   world_to_chunk_coords(i32 world_x, i32 world_y, i32* chunk_x, i32* chunk_y, i32* local_x, i32* local_y);

#endif
