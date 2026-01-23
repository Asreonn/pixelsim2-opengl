#include "physics/movement.h"
#include "world/world.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
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
    // Powders fall down, then diagonally
    if (grid_try_move_down(world, x, y)) return;

    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;

    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;

    f32 resistance = 1.0f + mat->friction * 2.0f + mat->viscosity * 0.2f;
    f32 spread_f = mat->dispersion / resistance;
    i32 spread = clampi((i32)roundf(spread_f), 1, MAX_SPREAD);
    if (grid_try_move_down_diagonal_spread(world, x, y, spread)) return;
    grid_try_move_horizontal(world, x, y, spread);
}

void movement_update_liquid(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;
    
    // Try to move down first
    if (grid_try_move_down(world, x, y)) return;

    i32 spread = movement_get_spread_distance(mat, pixel->temperature);
    bool down_left = grid_can_move_to(world, x, y, x - 1, y + 1);
    bool down_right = grid_can_move_to(world, x, y, x + 1, y + 1);

    if (down_left || down_right) {
        if (down_left && down_right) {
            if (grid_try_move_horizontal(world, x, y, spread)) return;
            if (grid_try_move_down_diagonal(world, x, y)) return;
        } else if (down_left) {
            if (grid_try_move(world, x, y, -1, 1)) return;
        } else {
            if (grid_try_move(world, x, y, 1, 1)) return;
        }
    }

    // Spread horizontally based on viscosity
    grid_try_move_horizontal(world, x, y, spread);
}

void movement_update_gas(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;
    
    // Gases rise (if temperature is high enough)
    if (pixel->temperature > AMBIENT_TEMPERATURE) {
        if (grid_try_move_up(world, x, y)) return;
        if (grid_try_move_up_diagonal(world, x, y)) return;
    } else {
        // Cold gas can sink
        u32 h = hash_position(x, y, world->tick);
        if ((h & 1u) != 0u) {
            if (grid_try_move_down(world, x, y)) return;
        }
    }
    
    // Spread horizontally
    i32 spread = movement_get_spread_distance(mat, pixel->temperature);
    grid_try_move_horizontal(world, x, y, spread);
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
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;
    
    switch (mat->base_state) {
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
