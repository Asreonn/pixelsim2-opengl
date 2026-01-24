#include "physics/movement.h"
#include "world/world.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "core/config.h"
#include "core/math_utils.h"
#include <math.h>

static bool movement_is_supported(World* world, i32 x, i32 y) {
    Pixel* below = world_get_pixel(world, x, y + 1);
    if (!below || below->material_id == MAT_AIR) return false;

    if (grid_can_move_to(world, x, y + 1, x, y + 2)) return false;
    if (grid_can_move_to(world, x, y + 1, x - 1, y + 2)) return false;
    if (grid_can_move_to(world, x, y + 1, x + 1, y + 2)) return false;

    return true;
}

void movement_apply_damping(Pixel* pixel, const MaterialDef* mat, f32 dt) {
    if (!pixel || !mat) return;
    
    f32 damping = 1.0f - (mat->friction * (1.0f + mat->viscosity * 0.1f) * dt);
    damping = clampf(damping, 0.0f, 1.0f);
    
    pixel->velocity_x *= damping;
    pixel->velocity_y *= damping;
}

void movement_apply_acceleration(Pixel* pixel, f32 ax, f32 ay, f32 dt) {
    if (!pixel) return;
    
    pixel->velocity_x += ax * dt;
    pixel->velocity_y += ay * dt;
    
    // Clamp velocities
    pixel->velocity_x = clampf(pixel->velocity_x, -MAX_VELOCITY, MAX_VELOCITY);
    pixel->velocity_y = clampf(pixel->velocity_y, -MAX_VELOCITY, MAX_VELOCITY);
}

f32 movement_get_effective_viscosity(const MaterialDef* mat, f32 temperature) {
    if (!mat) return 1.0f;
    
    // Viscosity decreases with temperature
    // Formula: eta_eff = eta * (T_ref / T)
    f32 temp_factor = 300.0f / (temperature + 100.0f);
    return mat->viscosity * temp_factor;
}

i32 movement_get_spread_distance(const MaterialDef* mat, f32 temperature) {
    if (!mat) return 1;
    
    f32 effective_viscosity = movement_get_effective_viscosity(mat, temperature);
    f32 spread = mat->dispersion / (1.0f + effective_viscosity);
    
    return clampi((i32)spread, 1, MAX_SPREAD);
}

void movement_update_solid(World* world, i32 x, i32 y) {
    // Solids don't move on their own
    (void)world;
    (void)x;
    (void)y;
}

void movement_update_powder(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;

    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;

    // === SICAK KUM HAREKETI KONTROLU ===
    // Kum icin ozel kontrol - sicaksa hareket etme (erimis kum olmayi bekle)
    if (pixel->material_id == MAT_SAND) {
        f32 melting_point = MAT_CACHE_MELTING_POINT(MAT_SAND);
        // Daha erken sabitlenme: %40 erime noktasinda dur
        if (pixel->temperature > melting_point * 0.4f) {
            // Ates, cam veya erimis kum komsusu var mi kontrol et
            for (i32 dir = 0; dir < 4; dir++) {
                Pixel* n = grid_get_neighbor(world, x, y, (Direction)dir);
                if (n && (n->material_id == MAT_GLASS ||
                          n->material_id == MAT_FIRE ||
                          n->material_id == MAT_MOLTEN_SAND)) {
                    return;  // Hareket etme, erimeyi bekle
                }
            }
            // Cok sicaksa (%60+) her durumda dur
            if (pixel->temperature > melting_point * 0.6f) {
                return;
            }
        }
    }

    // Stack pressure was pre-calculated in pressure_calculate_all_stacks()
    f32 stack_pressure = (pixel->pressure - AMBIENT_PRESSURE) / 15000.0f;

    // Slow powders down in liquids (drag + reduced step count)
    f32 sink_factor = 1.0f;
    u16 liquid_id = MAT_AIR;
    Pixel* below = world_get_pixel(world, x, y + 1);
    Pixel* above = world_get_pixel(world, x, y - 1);
    if (below && MAT_CACHE_IS_LIQUID(below->material_id)) {
        liquid_id = below->material_id;
    } else if (above && MAT_CACHE_IS_LIQUID(above->material_id)) {
        liquid_id = above->material_id;
    }

    if (liquid_id != MAT_AIR) {
        f32 fluid_density = MAT_CACHE_DENSITY(liquid_id);
        f32 fluid_viscosity = MAT_CACHE_VISCOSITY(liquid_id);
        f32 density_ratio = fluid_density / mat->density;
        f32 viscosity_factor = 1.0f + fluid_viscosity * 10.0f;

        sink_factor = clampf(1.0f - density_ratio * 0.8f, 0.15f, 0.7f);
        sink_factor /= viscosity_factor;

        f32 velocity_damp = clampf(0.55f + 0.45f * sink_factor, 0.35f, 0.9f);
        pixel->velocity_y *= velocity_damp;
    }

    f32 fall_factor = clampf(1.0f - (fabsf(pixel->velocity_y) / 2.5f), 0.0f, 1.0f);
    stack_pressure *= fall_factor;

    // Use velocity to determine how many steps to attempt
    i32 steps = clampi((i32)fabsf(pixel->velocity_y * sink_factor), 1, 5);
    
    i32 curr_x = x, curr_y = y;
    bool moved = false;
    
    for (i32 step = 0; step < steps; step++) {
        // Try to move straight down first
        if (grid_try_move_down(world, curr_x, curr_y)) {
            curr_y++;
            moved = true;
            continue;
        }
        
        // Can't move down - try diagonal

        // === TOZ ISLAKLIK KÜMELENMESİ ===
        // Yakındaki sıvıları say - ıslaklık viskoziteyi artırır
        i32 liquid_neighbors = 0;
        for (i32 dir = 0; dir < 4; dir++) {
            Pixel* n = grid_get_neighbor(world, curr_x, curr_y, (Direction)dir);
            if (n && MAT_CACHE_IS_LIQUID(n->material_id)) {
                liquid_neighbors++;
            }
        }
        f32 wetness_factor = 1.0f + liquid_neighbors * 0.15f;

        f32 resistance = 1.0f + mat->friction * 2.0f + mat->viscosity * 0.2f;
        f32 spread_f = mat->dispersion / (resistance * wetness_factor);
        i32 spread = clampi((i32)roundf(spread_f), 1, MAX_SPREAD);

        // === TOZ YIĞIN AÇISI ===
        // Sürtünme diyagonal kayma olasılığını belirler
        // Yüksek sürtünme = dik yığınlar, düşük sürtünme = düz yığınlar
        f32 angle_factor = mat->friction;
        u32 h = hash_position(curr_x, curr_y, world->tick);
        f32 sample = (f32)(h & 0xFFFFu) / 65535.0f;

        // Yüksek sürtünme = düşük kayma olasılığı (0.8 → 0.6 daha kolay kayma)
        if (sample > angle_factor * 0.6f) {
            if (grid_try_move_down_diagonal_spread(world, curr_x, curr_y, spread)) {
                pixel = world_get_pixel(world, curr_x, curr_y + 1);
                if (pixel) {
                    pixel->velocity_y *= 0.7f;
                }
                moved = true;
            }
        }
        break;
    }
    
    // Pressure-based spreading - use pre-calculated pressure
    if (stack_pressure > 0.3f && movement_is_supported(world, curr_x, curr_y)) {
        Pixel* current = world_get_pixel(world, curr_x, curr_y);
        if (current && current->material_id != MAT_AIR) {
            Pixel* below = world_get_pixel(world, curr_x, curr_y + 1);
            if (below && below->material_id != MAT_AIR) {
                bool pushed = grid_try_pressure_push(world, curr_x, curr_y, stack_pressure, mat->viscosity);

                if (!pushed && stack_pressure > 1.5f) {
                    grid_propagate_pressure_up(world, curr_x, curr_y, 6);
                }
            }
        }
    }
}

void movement_update_liquid(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;

    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;

    // Stack pressure was pre-calculated
    f32 stack_pressure = (pixel->pressure - AMBIENT_PRESSURE) / 15000.0f;
    f32 fall_factor = clampf(1.0f - (fabsf(pixel->velocity_y) / 3.0f), 0.0f, 1.0f);
    stack_pressure *= fall_factor;

    // === VİSKOZİTE BAZLI AKIŞ ===
    // Sıcaklığa bağlı efektif viskozite
    f32 eff_viscosity = movement_get_effective_viscosity(mat, pixel->temperature);

    // Viskozite adım sayısını ve yayılmayı etkiler
    f32 viscosity_penalty = 1.0f / (1.0f + eff_viscosity * 2.0f);
    i32 base_spread = movement_get_spread_distance(mat, pixel->temperature);
    i32 spread = clampi((i32)(base_spread * viscosity_penalty), 1, base_spread);

    // Liquids can move multiple cells when falling fast
    // Yüksek viskozite = daha az adım
    i32 base_steps = clampi((i32)fabsf(pixel->velocity_y), 1, 4);
    i32 steps = clampi((i32)(base_steps * viscosity_penalty), 1, base_steps);
    
    i32 curr_x = x, curr_y = y;
    bool moved = false;
    
    for (i32 step = 0; step < steps; step++) {
        if (grid_try_move_down(world, curr_x, curr_y)) {
            curr_y++;
            moved = true;
            continue;
        }
        
        bool down_left = grid_can_move_to(world, curr_x, curr_y, curr_x - 1, curr_y + 1);
        bool down_right = grid_can_move_to(world, curr_x, curr_y, curr_x + 1, curr_y + 1);
        
        if (down_left || down_right) {
            if (down_left && down_right) {
                if (grid_try_move_down_diagonal(world, curr_x, curr_y)) {
                    moved = true;
                    break;
                }
            } else if (down_left) {
                if (grid_try_move(world, curr_x, curr_y, -1, 1)) {
                    moved = true;
                    break;
                }
            } else {
                if (grid_try_move(world, curr_x, curr_y, 1, 1)) {
                    moved = true;
                    break;
                }
            }
        }
        
        // === YÜZEY GERİLİMİ (azaltılmış) ===
        // İzole sıvı damlacıkları daha az yayılır - eşik 3 → 2
        i32 same_neighbors = grid_count_neighbors_of_type(world, curr_x, curr_y, pixel->material_id);
        i32 surface_spread = spread;
        if (same_neighbors < 2) {
            surface_spread = clampi(spread - (2 - same_neighbors), 1, spread);
        }

        if (grid_try_move_horizontal(world, curr_x, curr_y, surface_spread)) {
            moved = true;
            // Horizontal movement completed - reduce x velocity
            Pixel* p = world_get_pixel(world, curr_x, curr_y);
            if (p) p->velocity_x *= 0.3f;
        }
        break;
    }

    // Apply damping based on whether liquid moved
    Pixel* final_pixel = world_get_pixel(world, curr_x, curr_y);
    if (final_pixel) {
        if (moved) {
            // Normal damping for moving liquid
            final_pixel->velocity_x *= 0.95f;
            final_pixel->velocity_y *= 0.95f;
        } else {
            // Aggressive damping when stuck - liquid should settle quickly
            final_pixel->velocity_x *= 0.5f;
            final_pixel->velocity_y *= 0.5f;

            // Zero out very small velocities to prevent micro-erosion
            if (fabsf(final_pixel->velocity_x) < 0.1f) final_pixel->velocity_x = 0;
            if (fabsf(final_pixel->velocity_y) < 0.1f) final_pixel->velocity_y = 0;
        }
    }

    // Liquids always try pressure push - they flow very easily
    if (stack_pressure > 0.2f && movement_is_supported(world, curr_x, curr_y)) {
        Pixel* below = world_get_pixel(world, curr_x, curr_y + 1);
        if (below && below->material_id != MAT_AIR) {
            f32 effective_viscosity = mat->viscosity * 0.3f;
            bool pushed = grid_try_pressure_push(world, curr_x, curr_y, stack_pressure + 0.5f, effective_viscosity);
            
            if (!pushed && stack_pressure > 1.0f) {
                grid_propagate_pressure_up(world, curr_x, curr_y, 8);
            }
        }
    }
}

void movement_update_gas(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;
    
    i32 spread = movement_get_spread_distance(mat, pixel->temperature);
    
    // Gas movement based on temperature difference from ambient
    // Hot gases rise faster, cold gases can sink
    f32 temp_diff = pixel->temperature - AMBIENT_TEMPERATURE;
    i32 steps = clampi((i32)(fabsf(temp_diff) / 50.0f) + 1, 1, 3);
    
    i32 curr_x = x, curr_y = y;
    for (i32 step = 0; step < steps; step++) {
        if (temp_diff > 0) {
            // Hot gas rises
            if (grid_try_move_up(world, curr_x, curr_y)) {
                curr_y--;
                continue;
            }
            if (grid_try_move_up_diagonal(world, curr_x, curr_y)) break;
        } else {
            // Cold gas can sink (with some randomness)
            u32 h = hash_position(curr_x, curr_y, world->tick + (u32)step);
            if ((h & 1u) != 0u) {
                if (grid_try_move_down(world, curr_x, curr_y)) {
                    curr_y++;
                    continue;
                }
            }
        }
        break;
    }

    // === GAZ TÜRBULANSI ===
    // Yüksek basınç farkı = kaotik hareket
    f32 neighbor_pressure = grid_get_average_neighbor_pressure(world, curr_x, curr_y);
    f32 pressure_diff = fabsf(pixel->pressure - neighbor_pressure);

    if (pressure_diff > 1000.0f) {
        // Türbülans: rastgele yatay hareket
        u32 h = hash_position(curr_x, curr_y, world->tick);
        if ((h % 3u) == 0u) {
            i32 turb_dx = ((h >> 4) & 1u) ? 1 : -1;
            grid_try_move(world, curr_x, curr_y, turb_dx, 0);
        }
    }

    // Spread horizontally
    grid_try_move_horizontal(world, curr_x, curr_y, spread);
}

void movement_update_plasma(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    // Plasma always rises and spreads
    if (grid_try_move_up(world, x, y)) return;
    if (grid_try_move_up_diagonal(world, x, y)) return;
    
    // Deterministic horizontal movement
    u32 h = hash_position(x, y, world->tick);
    if ((h % 10u) < 3u) {
        i32 dx = (h & 1u) ? 1 : -1;
        grid_try_move(world, x, y, dx, 0);
    }
}

void movement_update(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    // Use cached state for faster lookup
    MaterialState state = MAT_CACHE_STATE(pixel->material_id);
    
    switch (state) {
        case STATE_SOLID:
            movement_update_solid(world, x, y);
            break;
        case STATE_POWDER:
            movement_update_powder(world, x, y);
            break;
        case STATE_LIQUID:
            movement_update_liquid(world, x, y);
            break;
        case STATE_GAS:
            movement_update_gas(world, x, y);
            break;
        case STATE_PLASMA:
            movement_update_plasma(world, x, y);
            break;
        default:
            break;
    }
}
