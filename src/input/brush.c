#include "input/brush.h"
#include "world/world.h"
#include "materials/material_registry.h"
#include "core/config.h"
#include "core/math_utils.h"

static f32 brush_default_temperature(u16 material_id) {
    switch (material_id) {
        case MAT_FIRE:
            return FIRE_TEMPERATURE;
        case MAT_STEAM:
            return 400.0f;
        case MAT_ICE:
            return 260.0f;
        default:
            return AMBIENT_TEMPERATURE;
    }
}

void brush_init(Brush* brush) {
    brush->size = DEFAULT_BRUSH_SIZE;
    brush->material_id = MAT_SAND;
    brush->shape = BRUSH_CIRCLE;
    brush->temperature = brush_default_temperature(MAT_SAND);
    brush->pressure = AMBIENT_PRESSURE;
}

void brush_set_size(Brush* brush, i32 size) {
    brush->size = clampi(size, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE);
}

void brush_set_material(Brush* brush, u16 material_id) {
    if (material_id < material_count()) {
        brush->material_id = material_id;
        brush->temperature = brush_default_temperature(material_id);
    }
}

void brush_set_shape(Brush* brush, BrushShape shape) {
    brush->shape = shape;
}

void brush_set_temperature(Brush* brush, f32 temperature) {
    brush->temperature = clampf(temperature, MIN_TEMPERATURE, MAX_TEMPERATURE);
}

void brush_increase_size(Brush* brush) {
    brush_set_size(brush, brush->size + 1);
}

void brush_decrease_size(Brush* brush) {
    brush_set_size(brush, brush->size - 1);
}

void brush_next_material(Brush* brush) {
    u16 next = (brush->material_id + 1) % material_count();
    if (next == MAT_AIR) next = 1;  // Skip air
    brush->material_id = next;
}

void brush_prev_material(Brush* brush) {
    u16 prev = brush->material_id == 0 ? material_count() - 1 : brush->material_id - 1;
    if (prev == MAT_AIR) prev = material_count() - 1;
    brush->material_id = prev;
}

void brush_paint(Brush* brush, World* world, i32 x, i32 y) {
    i32 radius = brush->size;
    
    for (i32 dy = -radius; dy <= radius; dy++) {
        for (i32 dx = -radius; dx <= radius; dx++) {
            bool in_brush = false;
            
            if (brush->shape == BRUSH_CIRCLE) {
                in_brush = (dx * dx + dy * dy) <= (radius * radius);
            } else {
                in_brush = true;  // Square
            }
            
            if (in_brush) {
                i32 px = x + dx;
                i32 py = y + dy;
                
                if (world_in_bounds(px, py)) {
                    world_set_pixel_with_temp(world, px, py, brush->material_id, brush->temperature);
                }
            }
        }
    }
}

void brush_erase(Brush* brush, World* world, i32 x, i32 y) {
    i32 radius = brush->size;
    
    for (i32 dy = -radius; dy <= radius; dy++) {
        for (i32 dx = -radius; dx <= radius; dx++) {
            bool in_brush = false;
            
            if (brush->shape == BRUSH_CIRCLE) {
                in_brush = (dx * dx + dy * dy) <= (radius * radius);
            } else {
                in_brush = true;
            }
            
            if (in_brush) {
                i32 px = x + dx;
                i32 py = y + dy;
                
                if (world_in_bounds(px, py)) {
                    world_set_pixel(world, px, py, MAT_AIR);
                }
            }
        }
    }
}

const char* brush_get_material_name(Brush* brush) {
    return material_get_name(brush->material_id);
}
