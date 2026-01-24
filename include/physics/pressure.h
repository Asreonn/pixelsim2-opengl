#ifndef PIXELSIM_PRESSURE_H
#define PIXELSIM_PRESSURE_H

#include "core/types.h"

struct World;
struct Pixel;

// Calculate hydrostatic pressure at a position
// Formula: P = P0 + rho * g * h
f32 pressure_calculate_hydrostatic(struct World* world, i32 x, i32 y);

// Update pressure for gases (ideal gas law approximation)
// Formula: P * V = n * R * T  ->  P proportional to T/V
void pressure_update_gas(struct Pixel* pixel, f32 local_density, f32 dt);

// Calculate local pressure from neighbors
f32 pressure_calculate_local(struct World* world, i32 x, i32 y);

// Apply pressure forces to velocity
void pressure_apply_forces(struct World* world, i32 x, i32 y, f32 dt);

// Check for explosion conditions
bool pressure_check_explosion(struct World* world, i32 x, i32 y);

// Create explosion at position
void pressure_create_explosion(struct World* world, i32 x, i32 y, f32 power);

// Transfer pressure between pixels
void pressure_transfer(struct Pixel* p1, struct Pixel* p2, f32 rate, f32 dt);

// Pre-calculate stack pressure for movement (call before movement update)
void pressure_calculate_chunk_stack(struct World* world, i32 chunk_x, i32 chunk_y);
void pressure_calculate_all_stacks(struct World* world);

#endif
