#ifndef PIXELSIM_MATH_UTILS_H
#define PIXELSIM_MATH_UTILS_H

#include "types.h"

// Random number generation
void     rand_seed(u32 seed);
u32      rand_u32(void);
i32      rand_range(i32 min, i32 max);
f32      rand_float(void);
f32      rand_float_range(f32 min, f32 max);
bool     rand_chance(f32 probability);

// Math functions
f32      lerp(f32 a, f32 b, f32 t);
f32      clampf(f32 x, f32 lo, f32 hi);
i32      clampi(i32 x, i32 lo, i32 hi);
f32      signf(f32 x);
i32      signi(i32 x);
f32      absf(f32 x);
i32      absi(i32 x);

// Vector operations
Vec2     vec2_add(Vec2 a, Vec2 b);
Vec2     vec2_sub(Vec2 a, Vec2 b);
Vec2     vec2_scale(Vec2 v, f32 s);
f32      vec2_length(Vec2 v);
f32      vec2_length_sq(Vec2 v);
Vec2     vec2_normalize(Vec2 v);
f32      vec2_dot(Vec2 a, Vec2 b);

// Integer vector operations
Vec2i    vec2i_add(Vec2i a, Vec2i b);
Vec2i    vec2i_sub(Vec2i a, Vec2i b);

// Hash function for deterministic randomness
u32      hash_position(i32 x, i32 y, u64 tick);

// Color utilities
Color    color_lerp(Color a, Color b, f32 t);
Color    color_add_tint(Color base, Color tint, f32 amount);
Color    color_vary(Color base, u8 variation, u8 seed);

#endif
