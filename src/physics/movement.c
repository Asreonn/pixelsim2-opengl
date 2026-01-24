#include "physics/movement.h"
#include "world/world.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "core/config.h"
#include "core/math_utils.h"
#include <math.h>

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

    // Stack pressure was pre-calculated in pressure_calculate_all_stacks()
    f32 stack_pressure = (pixel->pressure - AMBIENT_PRESSURE) / 15000.0f;

    // Use velocity to determine how many steps to attempt
    i32 steps = clampi((i32)fabsf(pixel->velocity_y), 1, 5);
    
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
        f32 resistance = 1.0f + mat->friction * 2.0f + mat->viscosity * 0.2f;
        f32 spread_f = mat->dispersion / resistance;
        i32 spread = clampi((i32)roundf(spread_f), 1, MAX_SPREAD);
        
        if (grid_try_move_down_diagonal_spread(world, curr_x, curr_y, spread)) {
            pixel = world_get_pixel(world, curr_x, curr_y + 1);
            if (pixel) {
                pixel->velocity_y *= 0.7f;
            }
            moved = true;
        }
        break;
    }
    
    // Pressure-based spreading - use pre-calculated pressure
    if (stack_pressure > 0.3f) {
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
    
    i32 spread = movement_get_spread_distance(mat, pixel->temperature);
    
    // Liquids can move multiple cells when falling fast
    i32 steps = clampi((i32)fabsf(pixel->velocity_y), 1, 4);
    
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
        
        if (grid_try_move_horizontal(world, curr_x, curr_y, spread)) {
            moved = true;
        }
        break;
    }
    
    // Liquids always try pressure push - they flow very easily
    if (stack_pressure > 0.2f) {
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
