#ifndef PIXELSIM_COLOR_H
#define PIXELSIM_COLOR_H

#include "core/types.h"

struct Pixel;

// Calculate final display color for a pixel
Color color_calculate_pixel(const struct Pixel* pixel, bool show_temp, bool show_pressure);

// Temperature visualization color (blue -> red gradient)
Color color_from_temperature(f32 temperature);

// Pressure visualization color (green -> yellow -> red)
Color color_from_pressure(f32 pressure);

// Blend two colors
Color color_blend(Color a, Color b, f32 factor);

// Add variation to base color
Color color_add_variation(Color base, u8 variation_amount, u8 seed);

// Darken/lighten color
Color color_darken(Color c, f32 amount);
Color color_lighten(Color c, f32 amount);

#endif
