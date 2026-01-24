#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_MOLTEN_SAND = {
    .id = MAT_MOLTEN_SAND,
    .name = "Molten Sand",
    .base_state = STATE_LIQUID,          // Sivi gibi akar

    .mass = 0.1f,
    .density = 2200.0f,                  // Camdan biraz hafif
    .viscosity = 0.8f,                   // Yuksek viskozite (bal gibi)
    .friction = 0.3f,
    .elasticity = 0.0f,
    .dispersion = 2.0f,                  // Yavas yayilma

    .thermal_conductivity = 1.2f,
    .specific_heat = 900.0f,

    .melting_point = 0,                  // Zaten sivi
    .boiling_point = 2500.0f,            // Cok yuksek (buharlasmasaz)
    .freezing_point = 900.0f,            // Soguyunca cam olur
    .ignition_point = 0,

    .melts_into = MAT_AIR,
    .boils_into = MAT_AIR,
    .freezes_into = MAT_GLASS,           // Soguyunca cam
    .burns_into = MAT_AIR,

    .base_color = {255, 180, 80, 255},   // Parlak turuncu (erimis)
    .color_variation = 20,

    .flags = MAT_FLAG_EMITS_HEAT,        // Isi yayar
    .lifetime = 0,
    .custom_update = NULL
};
