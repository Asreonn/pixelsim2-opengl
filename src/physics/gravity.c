#include "physics/gravity.h"
#include "world/world.h"
#include "world/pixel.h"
#include "materials/material_registry.h"
#include "core/config.h"
#include "core/math_utils.h"

f32 gravity_calculate_acceleration(const MaterialDef* mat, f32 fluid_density, f32 gravity) {
    if (!mat || mat->density <= 0) return 0;
    
    // Buoyancy: a = g * (1 - rho_fluid / rho_object)
    f32 buoyancy_factor = 1.0f - (fluid_density / mat->density);
    return gravity * buoyancy_factor;
}

f32 gravity_calculate_thermal_lift(f32 pixel_temp, f32 ambient_temp) {
    if (ambient_temp <= 0) return 0;
    
    // Thermal lift: F = k * (T_pixel - T_ambient) / T_ambient
    f32 temp_diff = pixel_temp - ambient_temp;
    return THERMAL_LIFT_CONSTANT * (temp_diff / ambient_temp);
}

f32 gravity_get_net_acceleration(Pixel* pixel, World* world, i32 x, i32 y) {
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return 0;
    
    // Get fluid density from below (for falling) or above (for rising)
    f32 fluid_density = 1.2f; // Default air density
    
    Pixel* below = world_get_pixel(world, x, y + 1);
    if (below) {
        MaterialDef* below_mat = material_get(below->material_id);
        if (below_mat) {
            fluid_density = below_mat->density;
        }
    }
    
    // Base gravity with buoyancy
    f32 gravity_acc = gravity_calculate_acceleration(mat, fluid_density, world->gravity);
    
    // Thermal lift for hot materials
    f32 thermal_lift = gravity_calculate_thermal_lift(pixel->temperature, world->ambient_temperature);
    
    // Gases and plasma get extra thermal lift
    if (mat->base_state == STATE_GAS || mat->base_state == STATE_PLASMA) {
        thermal_lift *= 2.0f;
    }
    
    return gravity_acc - thermal_lift;
}

void gravity_apply(Pixel* pixel, World* world, i32 x, i32 y, f32 dt) {
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;
    
    // Solids don't move from gravity directly
    if (mat->base_state == STATE_SOLID) return;
    
    f32 acc = gravity_get_net_acceleration(pixel, world, x, y);
    
    // Update velocity
    pixel->velocity_y += acc * dt;
    
    // Clamp velocity
    pixel->velocity_y = clampf(pixel->velocity_y, -MAX_VELOCITY, MAX_VELOCITY);
    
    // Apply damping based on viscosity
    f32 effective_viscosity = mat->viscosity;
    if (pixel->temperature > 0) {
        effective_viscosity *= (300.0f / (pixel->temperature + 100.0f));
    }
    
    f32 damping = 1.0f - (mat->friction * effective_viscosity * dt);
    damping = clampf(damping, 0.0f, 1.0f);
    
    pixel->velocity_x *= damping;
    pixel->velocity_y *= damping;
}
