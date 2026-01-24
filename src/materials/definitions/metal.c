#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_METAL = {
    .id = MAT_METAL,
    .name = "Metal",
    .base_state = STATE_SOLID,

    .mass = 0.5f,
    .density = 7800.0f,           // Iron density
    .viscosity = 0,
    .friction = 0.5f,
    .elasticity = 0.3f,
    .dispersion = 0,

    .thermal_conductivity = 40.0f,  // Reduced heat conduction (was 80)
    .specific_heat = 450.0f,        // Iron: 449 J/(kg·K)

    .melting_point = 1800.0f,       // ~1538K iron, slightly higher
    .boiling_point = 0,
    .freezing_point = 0,
    .ignition_point = 0,

    .melts_into = MAT_AIR,          // TODO: Molten metal
    .boils_into = MAT_AIR,
    .freezes_into = MAT_AIR,
    .burns_into = MAT_AIR,

    .base_color = {160, 165, 175, 255},  // Silvery gray
    .color_variation = 8,

    .flags = MAT_FLAG_CONDUCTIVE,
    .lifetime = 0,
    .custom_update = NULL
};
