#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_SAND = {
    .id = MAT_SAND,
    .name = "Sand",
    .base_state = STATE_POWDER,
    
    .mass = 0.1f,
    .density = 1600.0f,
    .viscosity = 0.3f,       // 0.6 → 0.3 (daha akışkan)
    .friction = 0.50f,       // 0.55 → 0.50 (biraz daha kaygan)
    .elasticity = 0.0f,
    .dispersion = 2.0f,      // 1.5 → 2.0 (daha geniş yayılma)
    
    .thermal_conductivity = 4.0f,   // Higher for better heat absorption (was 2.0)
    .specific_heat = 830.0f,
    
    .melting_point = 1150.0f,
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,
    
    .melts_into = MAT_MOLTEN_SAND,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,
    
    .base_color = {240, 210, 150, 255},  // Warm sand
    .color_variation = 18,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
