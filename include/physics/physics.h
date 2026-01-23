#ifndef PIXELSIM_PHYSICS_H
#define PIXELSIM_PHYSICS_H

#include "core/types.h"
#include "world/world.h"

// Main physics update - processes all active chunks
void physics_update(World* world, f32 dt);

// Update single pixel
void physics_update_pixel(World* world, i32 x, i32 y, f32 dt);

// Process chunk physics
void physics_update_chunk(World* world, Chunk* chunk, f32 dt);

#endif
