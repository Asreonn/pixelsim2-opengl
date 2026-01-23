#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_STONE = {
    .id = MAT_STONE,
    .name = "Stone",
    .base_state = STATE_SOLID,
    
    .mass = 0.5f,
    .density = 2700.0f,
    .viscosity = 0,
    .friction = 0.7f,
    .elasticity = 0.1f,
    .dispersion = 0,
    
    .thermal_conductivity = 6.0f,
    .specific_heat = 650.0f,
    
    .melting_point = 1500.0f,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_AIR,  // Lava would go here
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {128, 128, 128, 255},  // Gray
    .color_variation = 30,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
