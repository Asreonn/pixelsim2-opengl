#include "physics/temperature.h"
#include "world/world.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "materials/material_ids.h"
#include "core/config.h"
#include "core/math_utils.h"

f32 temperature_effective_conductivity(f32 k1, f32 k2) {
    // Harmonic mean for conductivity between different materials
    if (k1 <= 0 || k2 <= 0) return 0;
    return (2.0f * k1 * k2) / (k1 + k2);
}

void temperature_update(World* world, i32 x, i32 y, f32 dt) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    u16 mat_id = pixel->material_id;
    f32 mat_thermal_k = MAT_CACHE_THERMAL_K(mat_id);
    f32 mat_density = MAT_CACHE_DENSITY(mat_id);
    f32 mat_specific_heat = MAT_CACHE_SPECIFIC_HEAT(mat_id);
    MaterialState mat_state = MAT_CACHE_STATE(mat_id);
    
    f32 total_heat_transfer = 0;
    i32 neighbor_count = 0;
    i32 air_neighbors = 0;
    
    // Heat transfer with 4 cardinal neighbors (Fourier's law)
    for (i32 i = 0; i < 4; i++) {
        Pixel* neighbor = grid_get_neighbor(world, x, y, (Direction)i);
        if (!neighbor) {
            air_neighbors++;
            continue;
        }

        if (neighbor->material_id == MAT_AIR) {
            air_neighbors++;
            continue;
        }
        
        f32 neighbor_thermal_k = MAT_CACHE_THERMAL_K(neighbor->material_id);
        
        // Effective thermal conductivity between the two materials
        f32 k_eff = temperature_effective_conductivity(mat_thermal_k, neighbor_thermal_k);
        
        // Heat transfer: Q = k_eff * (T_neighbor - T_pixel)
        f32 temp_diff = neighbor->temperature - pixel->temperature;
        total_heat_transfer += k_eff * temp_diff;
        neighbor_count++;
    }

    // Diagonal neighbors contribute smaller heat transfer
    for (i32 i = 4; i < 8; i++) {
        Pixel* neighbor = grid_get_neighbor(world, x, y, (Direction)i);
        if (!neighbor || neighbor->material_id == MAT_AIR) continue;

        f32 neighbor_thermal_k = MAT_CACHE_THERMAL_K(neighbor->material_id);
        f32 k_eff = temperature_effective_conductivity(mat_thermal_k, neighbor_thermal_k);

        f32 temp_diff = neighbor->temperature - pixel->temperature;
        total_heat_transfer += (k_eff * temp_diff) * 0.5f;
        neighbor_count++;
    }
    
    if (neighbor_count > 0) {
        // Thermal diffusivity: alpha = k / (rho * c)
        f32 alpha = mat_thermal_k / (mat_density * mat_specific_heat + 0.001f);
        alpha *= HEAT_TRANSFER_SCALE;

        // Apply heat transfer
        pixel->temperature += alpha * total_heat_transfer * dt;
    }

    // === SICAK MATERYAL EKSTRA ISI YAYIMI ===
    // Sıcak materyaller (>800K) komşularını ekstra ısıtır (conductivity bypass)
    if (pixel->temperature > 800.0f) {
        f32 excess_heat = (pixel->temperature - 800.0f) * 0.002f;
        for (i32 i = 0; i < 4; i++) {
            Pixel* neighbor = grid_get_neighbor(world, x, y, (Direction)i);
            if (neighbor && neighbor->material_id != MAT_AIR) {
                neighbor->temperature += excess_heat;
            }
        }
    }
    
    // Ambient exchange only if exposed to air
    if (air_neighbors > 0) {
        f32 exposure = (f32)air_neighbors / 4.0f;
        f32 exchange = COOLING_RATE;

        // === MATERYAL BAZLI SOĞUMA ORANLARI ===
        // Önce materyal-spesifik kontrol
        if (mat_id == MAT_MOLTEN_SAND) {
            // Erimiş kum çok hızlı soğumalı
            exchange *= 3.0f;
        } else if (mat_id == MAT_METAL && pixel->temperature > 400.0f) {
            // Sıcak metal hızlı soğur
            exchange *= 2.0f;
        } else if (mat_id == MAT_STEAM) {
            // Buhar yavaş soğur (geç yoğunlaşma)
            exchange *= 0.4f;
        } else {
            // Varsayılan state bazlı soğuma
            switch (mat_state) {
                case STATE_GAS:
                case STATE_PLASMA:
                    exchange *= 1.5f;
                    break;
                case STATE_SOLID:
                    exchange *= 0.7f;
                    break;
                case STATE_LIQUID:
                    exchange *= 1.1f;
                    break;
                default:
                    break;
            }
        }
        temperature_apply_cooling(pixel, world->ambient_temperature, exposure * exchange, dt);
    }
    
    // Clamp temperature
    pixel->temperature = clampf(pixel->temperature, MIN_TEMPERATURE, MAX_TEMPERATURE);
}

void temperature_transfer(Pixel* p1, Pixel* p2, f32 dt) {
    if (!p1 || !p2) return;
    
    MaterialDef* mat1 = material_get(p1->material_id);
    MaterialDef* mat2 = material_get(p2->material_id);
    if (!mat1 || !mat2) return;
    
    f32 k_eff = temperature_effective_conductivity(
        mat1->thermal_conductivity,
        mat2->thermal_conductivity
    );
    
    f32 temp_diff = p2->temperature - p1->temperature;
    f32 transfer = k_eff * temp_diff * dt * 0.1f;
    
    p1->temperature += transfer;
    p2->temperature -= transfer;
}

void temperature_apply_cooling(Pixel* pixel, f32 ambient_temp, f32 exposure, f32 dt) {
    if (!pixel) return;
    
    // Gradual cooling toward ambient
    f32 temp_diff = ambient_temp - pixel->temperature;
    pixel->temperature += temp_diff * exposure * dt;
}

void temperature_spread_heat(World* world, i32 x, i32 y, f32 heat_amount) {
    for (i32 i = 0; i < 4; i++) {
        Pixel* neighbor = grid_get_neighbor(world, x, y, (Direction)i);
        if (neighbor) {
            neighbor->temperature += heat_amount * 0.25f;
        }
    }
}

Color temperature_get_color_tint(f32 temperature) {
    f32 temp_c = temperature - 273.15f;
    f32 ambient_c = AMBIENT_TEMPERATURE - 273.15f;

    if (temp_c < ambient_c) {
        f32 t = (ambient_c - temp_c) / 80.0f;
        t = clampf(t, 0, 1);
        Color cold = (Color){20, 40, 140, 0};
        Color mid = (Color){80, 180, 255, 0};
        Color color = color_lerp(cold, mid, t);
        color.a = (u8)(t * 200);
        return color;
    }

    if (temp_c > ambient_c) {
        f32 t = (temp_c - ambient_c) / 500.0f;
        t = clampf(t, 0, 1);
        Color warm = (Color){255, 220, 80, 0};
        Color hot = (Color){255, 120, 20, 0};
        Color white = (Color){255, 255, 255, 0};
        Color color;

        if (t < 0.5f) {
            color = color_lerp(warm, hot, t * 2.0f);
        } else {
            color = color_lerp(hot, white, (t - 0.5f) * 2.0f);
        }
        color.a = (u8)(t * 220);
        return color;
    }

    return (Color){0, 0, 0, 0};
}
