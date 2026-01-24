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

    // Use velocity to determine how many steps to attempt
    // Faster falling pixels move more cells per tick
    i32 steps = clampi((i32)fabsf(pixel->velocity_y), 1, 5);
    
    i32 curr_x = x, curr_y = y;
    bool moved = false;
    
    for (i32 step = 0; step < steps; step++) {
        // Try to move straight down first
        if (grid_try_move_down(world, curr_x, curr_y)) {
            curr_y++;  // Update position for next step
            moved = true;
            continue;
        }
        
        // Can't move down - try diagonal
        f32 resistance = 1.0f + mat->friction * 2.0f + mat->viscosity * 0.2f;
        f32 spread_f = mat->dispersion / resistance;
        i32 spread = clampi((i32)roundf(spread_f), 1, MAX_SPREAD);
        
        if (grid_try_move_down_diagonal_spread(world, curr_x, curr_y, spread)) {
            // Diagonal move succeeded - reduce velocity since we hit something
            pixel = world_get_pixel(world, curr_x, curr_y + 1);  // Get at new position
            if (pixel) {
                pixel->velocity_y *= 0.7f;  // Energy loss on bounce
            }
            moved = true;
        }
        break;  // Couldn't move, stop trying
    }
    
    // If couldn't move naturally, check for pressure from above
    if (!moved) {
        i32 weight = grid_get_weight_above(world, curr_x, curr_y, 8);
        if (weight > 0) {
            grid_try_pressure_push(world, curr_x, curr_y, weight);
        }
    }
}

void movement_update_liquid(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;
    
    i32 spread = movement_get_spread_distance(mat, pixel->temperature);
    
    // Use velocity to determine movement steps
    // Liquids can move multiple cells when falling fast
    i32 steps = clampi((i32)fabsf(pixel->velocity_y), 1, 4);
    
    i32 curr_x = x, curr_y = y;
    bool moved = false;
    
    for (i32 step = 0; step < steps; step++) {
        // Try to move down first
        if (grid_try_move_down(world, curr_x, curr_y)) {
            curr_y++;
            moved = true;
            continue;
        }
        
        // Can't move straight down - check diagonals
        bool down_left = grid_can_move_to(world, curr_x, curr_y, curr_x - 1, curr_y + 1);
        bool down_right = grid_can_move_to(world, curr_x, curr_y, curr_x + 1, curr_y + 1);
        
        if (down_left || down_right) {
            if (down_left && down_right) {
                // Both open - use pressure-based diagonal
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
        
        // Couldn't move down at all - spread horizontally
        if (grid_try_move_horizontal(world, curr_x, curr_y, spread)) {
            moved = true;
        }
        break;
    }
    
    // If couldn't move naturally, check for pressure from above
    // Liquids respond more to pressure than powders
    if (!moved) {
        i32 weight = grid_get_weight_above(world, curr_x, curr_y, 10);
        if (weight > 0) {
            grid_try_pressure_push(world, curr_x, curr_y, weight + 1);  // +1 because liquids flow easier
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
