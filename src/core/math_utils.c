#include "core/math_utils.h"
#include <math.h>
#include <stdlib.h>

// Simple xorshift random number generator for determinism
static u32 rng_state = 12345;

void rand_seed(u32 seed) {
    rng_state = seed ? seed : 12345;
}

u32 rand_u32(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

i32 rand_range(i32 min, i32 max) {
    if (min >= max) return min;
    return min + (i32)(rand_u32() % (u32)(max - min + 1));
}

f32 rand_float(void) {
    return (f32)rand_u32() / (f32)0xFFFFFFFF;
}

f32 rand_float_range(f32 min, f32 max) {
    return min + rand_float() * (max - min);
}

bool rand_chance(f32 probability) {
    return rand_float() < probability;
}

f32 lerp(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

f32 clampf(f32 x, f32 lo, f32 hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

i32 clampi(i32 x, i32 lo, i32 hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

f32 signf(f32 x) {
    if (x > 0) return 1.0f;
    if (x < 0) return -1.0f;
    return 0.0f;
}

i32 signi(i32 x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

f32 absf(f32 x) {
    return x < 0 ? -x : x;
}

i32 absi(i32 x) {
    return x < 0 ? -x : x;
}

Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){ a.x + b.x, a.y + b.y };
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){ a.x - b.x, a.y - b.y };
}

Vec2 vec2_scale(Vec2 v, f32 s) {
    return (Vec2){ v.x * s, v.y * s };
}

f32 vec2_length(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

f32 vec2_length_sq(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

Vec2 vec2_normalize(Vec2 v) {
    f32 len = vec2_length(v);
    if (len < 0.0001f) return (Vec2){ 0, 0 };
    return vec2_scale(v, 1.0f / len);
}

f32 vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

Vec2i vec2i_add(Vec2i a, Vec2i b) {
    return (Vec2i){ a.x + b.x, a.y + b.y };
}

Vec2i vec2i_sub(Vec2i a, Vec2i b) {
    return (Vec2i){ a.x - b.x, a.y - b.y };
}

u32 hash_position(i32 x, i32 y, u64 tick) {
    u32 h = (u32)x * 374761393u + (u32)y * 668265263u + (u32)tick * 1013904223u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

Color color_lerp(Color a, Color b, f32 t) {
    t = clampf(t, 0.0f, 1.0f);
    return (Color){
        .r = (u8)(a.r + (i32)(b.r - a.r) * t),
        .g = (u8)(a.g + (i32)(b.g - a.g) * t),
        .b = (u8)(a.b + (i32)(b.b - a.b) * t),
        .a = (u8)(a.a + (i32)(b.a - a.a) * t)
    };
}

Color color_add_tint(Color base, Color tint, f32 amount) {
    amount = clampf(amount, 0.0f, 1.0f);
    return (Color){
        .r = (u8)clampi((i32)base.r + (i32)(tint.r * amount), 0, 255),
        .g = (u8)clampi((i32)base.g + (i32)(tint.g * amount), 0, 255),
        .b = (u8)clampi((i32)base.b + (i32)(tint.b * amount), 0, 255),
        .a = base.a
    };
}

Color color_vary(Color base, u8 variation, u8 seed) {
    if (variation == 0) return base;
    i32 offset = ((i32)seed - 128) * variation / 255;
    return (Color){
        .r = (u8)clampi((i32)base.r + offset, 0, 255),
        .g = (u8)clampi((i32)base.g + offset, 0, 255),
        .b = (u8)clampi((i32)base.b + offset, 0, 255),
        .a = base.a
    };
}
