#include "physics/phase_transition.h"
#include "physics/temperature.h"
#include "world/world.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "core/config.h"
#include "core/math_utils.h"

void phase_transform(World* world, i32 x, i32 y, u16 new_material) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    f32 old_temp = pixel->temperature;
    pixel_init_with_temp(pixel, new_material, old_temp);
    world_wake_chunk_at(world, x, y);
}

bool phase_check_transitions(World* world, i32 x, i32 y) {
    if (phase_check_combustion(world, x, y)) return true;
    if (phase_check_boiling(world, x, y)) return true;
    if (phase_check_melting(world, x, y)) return true;
    if (phase_check_freezing(world, x, y)) return true;
    if (phase_check_condensation(world, x, y)) return true;
    return false;
}

bool phase_check_melting(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return false;
    
    if (mat->melting_point > 0 && pixel->temperature >= mat->melting_point) {
        if (mat->melts_into != MAT_AIR) {
            phase_transform(world, x, y, mat->melts_into);
            return true;
        }
    }
    return false;
}

bool phase_check_freezing(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return false;
    
    if (mat->freezing_point > 0 && pixel->temperature <= mat->freezing_point) {
        if (mat->freezes_into != MAT_AIR) {
            phase_transform(world, x, y, mat->freezes_into);
            return true;
        }
    }
    return false;
}

bool phase_check_boiling(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return false;
    
    if (mat->boiling_point > 0 && pixel->temperature >= mat->boiling_point) {
        if (mat->boils_into != MAT_AIR) {
            phase_transform(world, x, y, mat->boils_into);
            return true;
        }
    }
    return false;
}

bool phase_check_condensation(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return false;
    
    // Condensation happens when gas cools below boiling point of what it came from
    // For steam specifically
    if (pixel->material_id == MAT_STEAM && pixel->temperature < 373.0f) {
        phase_transform(world, x, y, MAT_WATER);
        return true;
    }
    
    return false;
}

bool phase_check_combustion(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return false;
    
    // Check if flammable and above ignition point
    if ((mat->flags & MAT_FLAG_FLAMMABLE) && 
        mat->ignition_point > 0 && 
        pixel->temperature >= mat->ignition_point) {
        
        if (mat->burns_into != MAT_AIR) {
            phase_transform(world, x, y, mat->burns_into);
        } else {
            phase_transform(world, x, y, MAT_FIRE);
        }
        
        // Spread heat to neighbors
        temperature_spread_heat(world, x, y, FIRE_HEAT_OUTPUT);
        return true;
    }
    return false;
}

void phase_update_fire(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel || pixel->material_id != MAT_FIRE) return;
    
    // Decrease lifetime
    if (pixel->lifetime > 0) {
        pixel->lifetime--;
    } else {
        // Fire dies, becomes air or smoke
        pixel_reset(pixel);
        return;
    }
    
    // Fire emits heat
    pixel->temperature = FIRE_TEMPERATURE;
    temperature_spread_heat(world, x, y, FIRE_HEAT_OUTPUT * 0.5f);

    bool touching = false;
    for (i32 i = 0; i < 8; i++) {
        i32 nx = x + DIR_DX[i];
        i32 ny = y + DIR_DY[i];
        Pixel* neighbor = world_get_pixel(world, nx, ny);
        if (!neighbor || neighbor->material_id == MAT_AIR) continue;
        if (neighbor->material_id != MAT_FIRE) {
            touching = true;
        }
        neighbor->temperature += FIRE_HEAT_OUTPUT * 0.8f;
        world_wake_chunk_at(world, nx, ny);
        temperature_spread_heat(world, nx, ny, FIRE_HEAT_OUTPUT * 0.2f);
    }

    if (touching && pixel->lifetime > 0) {
        pixel->lifetime = (pixel->lifetime > 1) ? (pixel->lifetime - 1) : 0;
    }
    
    // Try to spread fire
    phase_spread_fire(world, x, y);
}

void phase_spread_fire(World* world, i32 x, i32 y) {
    for (i32 i = 0; i < 8; i++) {
        u32 h = hash_position(x + DIR_DX[i], y + DIR_DY[i], world->tick);
        if ((h % 10u) != 0u) continue; // 10% chance per neighbor
        
        i32 nx = x + DIR_DX[i];
        i32 ny = y + DIR_DY[i];
        
        Pixel* neighbor = world_get_pixel(world, nx, ny);
        if (!neighbor) continue;
        
        MaterialDef* mat = material_get(neighbor->material_id);
        if (!mat) continue;
        
        // Ignite flammable neighbors
        if ((mat->flags & MAT_FLAG_FLAMMABLE) && neighbor->material_id != MAT_FIRE) {
            neighbor->temperature += FIRE_HEAT_OUTPUT;
        }
    }
}
