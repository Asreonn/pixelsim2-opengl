#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_WATER = {
    .id = MAT_WATER,
    .name = "Water",
    .base_state = STATE_LIQUID,
    
    .mass = 0.1f,
    .density = 1000.0f,
    .viscosity = 0.03f,
    .friction = 0.04f,
    .elasticity = 0.0f,
    .dispersion = 7.0f,      // 5.0 → 7.0 (daha hızlı yayılma)
    
    .thermal_conductivity = 1.8f,
    .specific_heat = 3000.0f,
    
    .melting_point = 0,
    .boiling_point = 373.0f,
    .freezing_point = 273.0f,
    .ignition_point = 0,
    
    .melts_into = MAT_AIR,
    .boils_into = MAT_STEAM,
    .freezes_into = MAT_ICE,
    .burns_into = MAT_AIR,
    
    .base_color = {50, 170, 235, 215},  // Fresh aqua
    .color_variation = 12,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
