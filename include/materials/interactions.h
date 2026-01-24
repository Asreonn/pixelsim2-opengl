#ifndef PIXELSIM_INTERACTIONS_H
#define PIXELSIM_INTERACTIONS_H

#include "core/types.h"
#include "materials/material_ids.h"

// Material interaction definition
typedef struct MaterialInteraction {
    u16 material_a;
    u16 material_b;
    u16 result_a;              // What material_a becomes
    u16 result_b;              // What material_b becomes
    InteractionType type;
    f32 probability;           // 0.0 - 1.0
    f32 min_temperature;       // Minimum temp required (Kelvin)
    f32 min_pressure;          // Minimum pressure required (Pa), 0 = no requirement
} MaterialInteraction;

// Initialize interaction system
void interactions_init(void);
void interactions_shutdown(void);

// Register an interaction
bool interaction_register(const MaterialInteraction* interaction);

// Shorthand registration
bool interaction_add(u16 mat_a, u16 mat_b, u16 result_a, u16 result_b, 
                     InteractionType type, f32 probability);

// Query interactions
const MaterialInteraction* interaction_find(u16 mat_a, u16 mat_b);
bool interaction_check(u16 mat_a, u16 mat_b, f32 temperature);

// Process interaction between two pixels
struct World;
bool interaction_process(struct World* world, i32 x1, i32 y1, i32 x2, i32 y2);

#endif
