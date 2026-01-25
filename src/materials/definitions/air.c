#include "materials/material_def.h"
#include "materials/material_ids.h"
#include "core/config.h"

MaterialDef MAT_DEF_AIR = {
    .id = MAT_AIR,
    .name = "Air",
    .base_state = STATE_GAS,
    
    .mass = 0.001f,
    .density = 1.2f,
    .viscosity = 0.01f,
    .friction = 0.0f,
    .elasticity = 0.0f,
    .dispersion = 5.0f,
    
    .thermal_conductivity = 0.5f,   // Simülasyon için artırıldı
    .specific_heat = 1005.0f,
    
    .melting_point = 0,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_AIR,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {200, 220, 255, 0},  // Transparent
    .color_variation = 0,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
