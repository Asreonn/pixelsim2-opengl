#include "physics/phase_transition.h"
#include "physics/temperature.h"
#include "world/world.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
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
    
    u16 mat_id = pixel->material_id;
    f32 melting_point = MAT_CACHE_MELTING_POINT(mat_id);
    
    if (melting_point > 0 && pixel->temperature >= melting_point) {
        u16 melts_into = MAT_CACHE_MELTS_INTO(mat_id);
        if (melts_into != MAT_AIR) {
            phase_transform(world, x, y, melts_into);
            return true;
        }
    }
    return false;
}

bool phase_check_freezing(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;

    u16 mat_id = pixel->material_id;
    f32 freezing_point = MAT_CACHE_FREEZING_POINT(mat_id);

    if (freezing_point > 0 && pixel->temperature <= freezing_point) {
        u16 freezes_into = MAT_CACHE_FREEZES_INTO(mat_id);
        if (freezes_into != MAT_AIR) {
            // Ekzotermik: Donma sırasında komşulara ısı ver
            // (Molten sand -> glass latent heat release)
            f32 latent_heat = 50.0f;  // Isı miktarı
            for (i32 i = 0; i < 4; i++) {
                Pixel* neighbor = grid_get_neighbor(world, x, y, (Direction)i);
                if (neighbor && neighbor->material_id != MAT_AIR) {
                    neighbor->temperature += latent_heat * 0.25f;
                }
            }
            phase_transform(world, x, y, freezes_into);
            return true;
        }
    }
    return false;
}

bool phase_check_boiling(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    u16 mat_id = pixel->material_id;
    f32 boiling_point = MAT_CACHE_BOILING_POINT(mat_id);
    
    if (boiling_point > 0 && pixel->temperature >= boiling_point) {
        u16 boils_into = MAT_CACHE_BOILS_INTO(mat_id);
        if (boils_into != MAT_AIR) {
            phase_transform(world, x, y, boils_into);
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
    
    u16 mat_id = pixel->material_id;
    u32 flags = MAT_CACHE_FLAGS(mat_id);
    f32 ignition_point = MAT_CACHE_IGNITION_POINT(mat_id);
    
    // Check if flammable and above ignition point
    if ((flags & MAT_FLAG_FLAMMABLE) && 
        ignition_point > 0 && 
        pixel->temperature >= ignition_point) {
        
        u16 burns_into = MAT_CACHE_BURNS_INTO(mat_id);
        if (burns_into != MAT_AIR) {
            phase_transform(world, x, y, burns_into);
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
        // Kuma biraz daha fazla ısı ama yavaş (0.8 → 1.0)
        f32 heat_mult = (neighbor->material_id == MAT_SAND) ? 1.0f : 0.8f;
        neighbor->temperature += FIRE_HEAT_OUTPUT * heat_mult;
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

        // Ignite flammable neighbors (use cached flags)
        if (MAT_CACHE_IS_FLAMMABLE(neighbor->material_id) && neighbor->material_id != MAT_FIRE) {
            neighbor->temperature += FIRE_HEAT_OUTPUT;
        }
    }
}

void phase_update_molten_sand(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel || pixel->material_id != MAT_MOLTEN_SAND) return;

    // Erimiş kum komşu kumları ısıtır ve camdan soğur (8 yön)
    for (i32 i = 0; i < 8; i++) {
        i32 nx = x + DIR_DX[i];
        i32 ny = y + DIR_DY[i];
        Pixel* neighbor = world_get_pixel(world, nx, ny);
        if (!neighbor) continue;

        if (neighbor->material_id == MAT_SAND) {
            // Kuma direk ısı ver - zincir ısıtma
            neighbor->temperature += 15.0f;
            world_wake_chunk_at(world, nx, ny);
        } else if (neighbor->material_id == MAT_GLASS) {
            // Cam erimiş kumu hızlı soğutur (zincir soğutma)
            f32 temp_diff = pixel->temperature - neighbor->temperature;
            if (temp_diff > 50.0f) {
                pixel->temperature -= temp_diff * 0.02f;  // %2 soğuma
            }
        }
    }
}
