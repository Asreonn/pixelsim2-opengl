#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_SAND = {
    .id = MAT_SAND,
    .name = "Sand",
    .base_state = STATE_POWDER,
    
    .mass = 0.1f,
    .density = 1600.0f,
    .viscosity = 1.0f,
    .friction = 0.55f,
    .elasticity = 0.0f,
    .dispersion = 4.0f,
    
    .thermal_conductivity = 0.6f,
    .specific_heat = 830.0f,
    
    .melting_point = 1150.0f,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_GLASS,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {240, 210, 150, 255},  // Warm sand
    .color_variation = 18,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
