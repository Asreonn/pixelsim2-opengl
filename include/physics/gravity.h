#ifndef PIXELSIM_GRAVITY_H
#define PIXELSIM_GRAVITY_H

#include "core/types.h"

struct World;
struct Pixel;
struct MaterialDef;

// Calculate gravity acceleration with buoyancy
// Formula: a = g * (1 - density_fluid / density_object)
f32 gravity_calculate_acceleration(const struct MaterialDef* mat, f32 fluid_density, f32 gravity);

// Calculate thermal lift force (hot air rises)
// Formula: F_thermal = k * (T_pixel - T_ambient) / T_ambient
f32 gravity_calculate_thermal_lift(f32 pixel_temp, f32 ambient_temp);

// Combined gravity + thermal lift
f32 gravity_get_net_acceleration(struct Pixel* pixel, struct World* world, i32 x, i32 y);

// Apply gravity to pixel velocity
void gravity_apply(struct Pixel* pixel, struct World* world, i32 x, i32 y, f32 dt);

#endif
