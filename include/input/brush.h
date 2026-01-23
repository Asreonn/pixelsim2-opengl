#ifndef PIXELSIM_BRUSH_H
#define PIXELSIM_BRUSH_H

#include "core/types.h"
#include "materials/material_ids.h"

struct World;

// Brush for drawing
typedef struct Brush {
    i32 size;                    // Radius in pixels
    u16 material_id;             // Material to place
    BrushShape shape;            // Circle or square
    f32 temperature;             // Initial temperature for placed pixels
    f32 pressure;                // Initial pressure for placed pixels
} Brush;

// Initialize brush with defaults
void brush_init(Brush* brush);

// Set brush properties
void brush_set_size(Brush* brush, i32 size);
void brush_set_material(Brush* brush, u16 material_id);
void brush_set_shape(Brush* brush, BrushShape shape);
void brush_set_temperature(Brush* brush, f32 temperature);

// Adjust brush size
void brush_increase_size(Brush* brush);
void brush_decrease_size(Brush* brush);

// Cycle through materials
void brush_next_material(Brush* brush);
void brush_prev_material(Brush* brush);

// Apply brush to world at position
void brush_paint(Brush* brush, struct World* world, i32 x, i32 y);
void brush_erase(Brush* brush, struct World* world, i32 x, i32 y);

// Get brush info
const char* brush_get_material_name(Brush* brush);

#endif
