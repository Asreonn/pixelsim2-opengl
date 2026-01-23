#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_GLASS = {
    .id = MAT_GLASS,
    .name = "Glass",
    .base_state = STATE_SOLID,
    
    .mass = 0.2f,
    .density = 2500.0f,
    .viscosity = 0,
    .friction = 0.3f,
    .elasticity = 0.0f,
    .dispersion = 0,
    
    .thermal_conductivity = 1.0f,
    .specific_heat = 840.0f,
    
    .melting_point = 1400.0f,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_AIR,  // Would be molten glass
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {200, 230, 255, 180},  // Transparent blueish
    .color_variation = 10,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
