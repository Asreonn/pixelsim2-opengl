#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_STEAM = {
    .id = MAT_STEAM,
    .name = "Steam",
    .base_state = STATE_GAS,
    
    .mass = 0.001f,
    .density = 0.6f,
    .viscosity = 0.01f,
    .friction = 0.0f,
    .elasticity = 0.0f,
    .dispersion = 5.0f,
    
    .thermal_conductivity = 0.02f,
    .specific_heat = 2010.0f,
    
    .melting_point = 0,
    .boiling_point = 0,
    .freezing_point = 373.0f,  // Condenses below 100C
    .ignition_point = 0,
    
    .melts_into = MAT_AIR,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_WATER,
    .burns_into = MAT_AIR,
    
    .base_color = {210, 220, 240, 170},  // Soft blue-gray
    .color_variation = 20,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
