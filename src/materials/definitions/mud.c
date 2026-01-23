#include "materials/material_def.h"
#include "materials/material_ids.h"

MaterialDef MAT_DEF_MUD = {
    .id = MAT_MUD,
    .name = "Mud",
    .base_state = STATE_POWDER,
    
    .mass = 0.15f,
    .density = 1800.0f,
    .viscosity = 3.5f,
    .friction = 0.8f,
    .elasticity = 0.0f,
    .dispersion = 1.5f,
    
    .thermal_conductivity = 0.5f,
    .specific_heat = 2000.0f,
    
    .melting_point = 0,
    .boiling_point = 373.0f,
    .freezing_point = 273.0f,
    .ignition_point = 0,
    
    .melts_into = MAT_AIR,
    .boils_into = MAT_STEAM,
    .freezes_into = MAT_MUD,  // Frozen mud stays mud
    .burns_into = MAT_AIR,
    
    .base_color = {101, 67, 33, 255},  // Brown
    .color_variation = 25,
    
    .flags = MAT_FLAG_NONE,
    .lifetime = 0,
    .custom_update = NULL
};
