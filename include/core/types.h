#ifndef PIXELSIM_TYPES_H
#define PIXELSIM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Basic Types
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;

// Vector Types
typedef struct Vec2 { f32 x, y; } Vec2;
typedef struct Vec2i { i32 x, y; } Vec2i;
typedef struct Color { u8 r, g, b, a; } Color;

// Material State
typedef enum MaterialState {
    STATE_SOLID = 0,
    STATE_POWDER,
    STATE_LIQUID,
    STATE_GAS,
    STATE_PLASMA,
    STATE_COUNT
} MaterialState;

// Material Flags
#define MAT_FLAG_NONE         0
#define MAT_FLAG_FLAMMABLE    (1 << 0)
#define MAT_FLAG_CONDUCTIVE   (1 << 1)
#define MAT_FLAG_EXPLOSIVE    (1 << 2)
#define MAT_FLAG_CORROSIVE    (1 << 3)
#define MAT_FLAG_EMITS_LIGHT  (1 << 4)
#define MAT_FLAG_EMITS_HEAT   (1 << 5)
#define MAT_FLAG_ABSORBS_HEAT (1 << 6)

// Interaction Types
typedef enum InteractionType {
    INTERACT_NONE = 0,
    INTERACT_COMBINE,
    INTERACT_REACT,
    INTERACT_DISSOLVE,
    INTERACT_SPREAD,
    INTERACT_COUNT
} InteractionType;

// Direction
typedef enum Direction {
    DIR_UP = 0, DIR_DOWN, DIR_LEFT, DIR_RIGHT,
    DIR_UP_LEFT, DIR_UP_RIGHT, DIR_DOWN_LEFT, DIR_DOWN_RIGHT,
    DIR_COUNT
} Direction;

static const i32 DIR_DX[DIR_COUNT] = { 0, 0, -1, 1, -1, 1, -1, 1 };
static const i32 DIR_DY[DIR_COUNT] = { -1, 1, 0, 0, -1, -1, 1, 1 };

// Brush Shape
typedef enum BrushShape { BRUSH_CIRCLE = 0, BRUSH_SQUARE } BrushShape;

// Macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN(MAX(x, lo), hi))
#define COLOR_RGBA(r, g, b, a) ((Color){(r), (g), (b), (a)})
#define COLOR_RGB(r, g, b) COLOR_RGBA(r, g, b, 255)
#define COLOR_TO_U32(c) (((u32)(c).a << 24) | ((u32)(c).b << 16) | ((u32)(c).g << 8) | (u32)(c).r)

#endif
