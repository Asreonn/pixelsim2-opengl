#include "materials/material_def.h"
#include "materials/material_ids.h"
#include "core/config.h"
#include "world/pixel.h"
#include "world/world.h"
#include "physics/phase_transition.h"

static void fire_update(Pixel* pixel, World* world, i32 x, i32 y) {
    phase_update_fire(world, x, y);
    (void)pixel;
}

MaterialDef MAT_DEF_FIRE = {
    .id = MAT_FIRE,
    .name = "Fire",
    .base_state = STATE_PLASMA,
    
    .mass = 0.001f,
    .density = 0.3f,
    .viscosity = 0.01f,
    .friction = 0.0f,
    .elasticity = 0.0f,
    .dispersion = 3.0f,
    
    .thermal_conductivity = 0.1f,
    .specific_heat = 1000.0f,
    
    .melting_point = 0,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_AIR,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {255, 100, 0, 255},  // Orange-red
    .color_variation = 50,
    
    .flags = MAT_FLAG_EMITS_HEAT | MAT_FLAG_EMITS_LIGHT,
    .lifetime = 30,  // Burns for about 0.5 second
    .custom_update = fire_update
};
