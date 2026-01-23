#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_ICE = {
    .id = MAT_ICE,
    .name = "Ice",
    .base_state = STATE_SOLID,
    
    .mass = 0.1f,
    .density = 917.0f,
    .viscosity = 0,
    .friction = 0.05f,
    .elasticity = 0.1f,
    .dispersion = 0,
    
    .thermal_conductivity = 4.0f,
    .specific_heat = 1600.0f,
    
    .melting_point = 273.0f,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_WATER,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {180, 220, 255, 230},  // Light blue, slightly transparent
    .color_variation = 15,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
