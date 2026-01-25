#include "physics/temperature.h"
#include "world/world.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "materials/material_ids.h"
#include "core/config.h"
#include "core/math_utils.h"
#include <math.h>

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
            // continue kaldırıldı - hava da ısı transferine dahil
        }
        
        f32 neighbor_thermal_k = MAT_CACHE_THERMAL_K(neighbor->material_id);
        
        // Effective thermal conductivity between the two materials
        f32 k_eff = temperature_effective_conductivity(mat_thermal_k, neighbor_thermal_k);
        
        // Heat transfer: Q = k_eff * (T_neighbor - T_pixel) * speed_factor
        f32 temp_diff = neighbor->temperature - pixel->temperature;
        f32 speed_factor = 1.0f + fabsf(temp_diff) / 1000.0f;  // Fark arttıkça hız artar
        total_heat_transfer += k_eff * temp_diff * speed_factor;
        neighbor_count++;
    }

    // Diagonal neighbors contribute smaller heat transfer
    for (i32 i = 4; i < 8; i++) {
        Pixel* neighbor = grid_get_neighbor(world, x, y, (Direction)i);
        if (!neighbor || neighbor->material_id == MAT_AIR) continue;

        f32 neighbor_thermal_k = MAT_CACHE_THERMAL_K(neighbor->material_id);
        f32 k_eff = temperature_effective_conductivity(mat_thermal_k, neighbor_thermal_k);

        f32 temp_diff = neighbor->temperature - pixel->temperature;
        f32 speed_factor = 1.0f + fabsf(temp_diff) / 1000.0f;
        total_heat_transfer += (k_eff * temp_diff * speed_factor) * 0.5f;
        neighbor_count++;
    }
    
    if (neighbor_count > 0) {
        // Thermal diffusivity: alpha = k / (rho * c)
        f32 alpha = mat_thermal_k / (mat_density * mat_specific_heat + 0.001f);
        alpha *= HEAT_TRANSFER_SCALE;

        // Apply heat transfer
        pixel->temperature += alpha * total_heat_transfer * dt;
    }

    // Ambient cooling - sadece havaya temas eden pikseller soğur
    f32 exposure = (f32)air_neighbors / 4.0f;
    if (exposure > 0.0f) {
        f32 exchange = COOLING_RATE;

        // Materyal bazlı çarpanlar
        if (mat_id == MAT_MOLTEN_SAND) {
            exchange *= 5.0f;
        } else if (mat_id == MAT_STEAM) {
            exchange *= 0.4f;
        } else {
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

    // Simetrik soğuma/ısınma
    f32 rate = 1.0f;

    pixel->temperature += temp_diff * exposure * rate * dt;
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
    f32 t;

    if (temperature < 200.0f) {
        // Çok soğuk: koyu mavi
        return (Color){20, 40, 180, 200};
    } else if (temperature < AMBIENT_TEMPERATURE) {
        // Soğuk: mavi → cyan
        t = (temperature - 200.0f) / (AMBIENT_TEMPERATURE - 200.0f);
        t = clampf(t, 0, 1);
        return (Color){
            (u8)(20 + 60 * t),
            (u8)(40 + 160 * t),
            (u8)(180 - 20 * t),
            (u8)(180 - 80 * t)
        };
    } else if (temperature < 400.0f) {
        // Ilık: yeşil → sarı
        t = (temperature - AMBIENT_TEMPERATURE) / (400.0f - AMBIENT_TEMPERATURE);
        t = clampf(t, 0, 1);
        return (Color){
            (u8)(80 + 175 * t),
            (u8)(200 - 55 * t),
            20,
            (u8)(100 + 100 * t)
        };
    } else if (temperature < 800.0f) {
        // Sıcak: sarı → turuncu
        t = (temperature - 400.0f) / 400.0f;
        t = clampf(t, 0, 1);
        return (Color){
            255,
            (u8)(145 - 95 * t),
            20,
            (u8)(200 + 30 * t)
        };
    } else if (temperature < 1200.0f) {
        // Çok sıcak: turuncu → kırmızı
        t = (temperature - 800.0f) / 400.0f;
        t = clampf(t, 0, 1);
        return (Color){
            255,
            (u8)(50 - 30 * t),
            (u8)(20 * t),
            230
        };
    } else {
        // Aşırı sıcak: kırmızı → beyaz
        t = clampf((temperature - 1200.0f) / 800.0f, 0, 1);
        return (Color){
            255,
            (u8)(20 + 235 * t),
            (u8)(20 + 235 * t),
            255
        };
    }
}
