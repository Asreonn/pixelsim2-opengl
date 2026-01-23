#ifndef PIXELSIM_TEMPERATURE_H
#define PIXELSIM_TEMPERATURE_H

#include "core/types.h"

struct World;
struct Pixel;

// Update temperature for a single pixel (Fourier heat transfer)
// Formula: dT = alpha * sum(T_neighbor - T_pixel) * dt
// where alpha = k / (density * specific_heat)
void temperature_update(struct World* world, i32 x, i32 y, f32 dt);

// Calculate effective thermal conductivity between two materials
// Formula: k_eff = 2 * k1 * k2 / (k1 + k2)
f32 temperature_effective_conductivity(f32 k1, f32 k2);

// Transfer heat between two pixels
void temperature_transfer(struct Pixel* p1, struct Pixel* p2, f32 dt);

// Apply ambient exchange based on exposure to air
// Formula: dT = (T_ambient - T_pixel) * exchange * dt
void temperature_apply_cooling(struct Pixel* pixel, f32 ambient_temp, f32 exposure, f32 dt);

// Heat spreading from heat-emitting materials (fire)
void temperature_spread_heat(struct World* world, i32 x, i32 y, f32 heat_amount);

// Get temperature-based color tint
Color temperature_get_color_tint(f32 temperature);

#endif
