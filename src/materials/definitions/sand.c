#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_SAND = {
    .id = MAT_SAND,
    .name = "Sand",
    .base_state = STATE_POWDER,
    
    .mass = 0.1f,
    .density = 1600.0f,
    .viscosity = 0.8f,
    .friction = 0.45f,
    .elasticity = 0.0f,
    .dispersion = 3.0f,
    
    .thermal_conductivity = 0.25f,
    .specific_heat = 830.0f,
    
    .melting_point = 1700.0f,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_GLASS,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {230, 200, 90, 255},  // Vibrant sand
    .color_variation = 20,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
