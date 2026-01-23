#ifndef PIXELSIM_MOVEMENT_H
#define PIXELSIM_MOVEMENT_H

#include "core/types.h"

struct World;
struct Pixel;
struct MaterialDef;

// Update pixel velocity with damping
// Formula: v_new = v_old * (1 - friction * viscosity_factor)
void movement_apply_damping(struct Pixel* pixel, const struct MaterialDef* mat, f32 dt);

// Update velocity from acceleration
void movement_apply_acceleration(struct Pixel* pixel, f32 ax, f32 ay, f32 dt);

// Calculate viscosity factor based on temperature
// Formula: effective_viscosity = base_viscosity * (T_ref / T_current)
f32 movement_get_effective_viscosity(const struct MaterialDef* mat, f32 temperature);

// Calculate spread distance based on viscosity
// Formula: spread = max_spread / (1 + viscosity)
i32 movement_get_spread_distance(const struct MaterialDef* mat, f32 temperature);

// Process movement for different material states
void movement_update_solid(struct World* world, i32 x, i32 y);
void movement_update_powder(struct World* world, i32 x, i32 y);
void movement_update_liquid(struct World* world, i32 x, i32 y);
void movement_update_gas(struct World* world, i32 x, i32 y);
void movement_update_plasma(struct World* world, i32 x, i32 y);

// Generic movement based on pixel's material state
void movement_update(struct World* world, i32 x, i32 y);

#endif
