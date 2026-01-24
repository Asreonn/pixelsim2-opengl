#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_MUD = {
    .id = MAT_MUD,
    .name = "Mud",
    .base_state = STATE_POWDER,
    
    .mass = 0.15f,
    .density = 1650.0f,      // Islak kum kuru kumdan hafif yoğun
    .viscosity = 1.5f,       // Yapışkan, akışa direnir
    .friction = 0.75f,       // Yüksek sürtünme - dik yığınlar
    .elasticity = 0.0f,
    .dispersion = 2.0f,      // Yavaş yayılır
    
    .thermal_conductivity = 0.5f,
    .specific_heat = 2000.0f,
    
    .melting_point = 0,
    .boiling_point = 450.0f,   // Sıcak çamur kurur → kum olur
    .freezing_point = 273.0f,
    .ignition_point = 0,

    .melts_into = MAT_AIR,
    .boils_into = MAT_SAND,    // Kuruma: çamur → kum
    .freezes_into = MAT_MUD,  // Frozen mud stays mud
    .burns_into = MAT_AIR,
    
    .base_color = {140, 90, 50, 255},  // Earthy clay
    .color_variation = 18,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
