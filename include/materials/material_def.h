#ifndef PIXELSIM_MATERIAL_DEF_H
#define PIXELSIM_MATERIAL_DEF_H

#include "core/types.h"
#include "material_ids.h"

// Forward declarations
struct Pixel;
struct World;

// Custom update function type
typedef void (*MaterialUpdateFn)(struct Pixel* pixel, struct World* world, i32 x, i32 y);

// Material Definition - static properties for each material type
typedef struct MaterialDef {
    // Identity
    u16 id;
    char name[32];
    MaterialState base_state;
    
    // Physical Properties
    f32 mass;                    // kg per pixel
    f32 density;                 // kg/m³
    f32 viscosity;               // Pa·s (flow resistance, 0 = flows easily)
    f32 friction;                // 0-1 (surface friction)
    f32 elasticity;              // 0-1 (bounciness)
    f32 dispersion;              // How much it spreads horizontally
    
    // Thermal Properties
    f32 thermal_conductivity;    // W/(m·K) - how fast heat transfers
    f32 specific_heat;           // J/(kg·K) - energy to raise 1K
    
    // Phase Transition Temperatures (Kelvin)
    f32 melting_point;           // Solid -> Liquid
    f32 boiling_point;           // Liquid -> Gas
    f32 freezing_point;          // Liquid -> Solid
    f32 ignition_point;          // Catches fire
    
    // Phase Transition Results (MaterialID)
    u16 melts_into;
    u16 boils_into;
    u16 freezes_into;
    u16 burns_into;
    
    // Visual Properties
    Color base_color;
    u8 color_variation;          // Random variation amount (0-255)
    
    // Behavior
    u32 flags;                   // MAT_FLAG_* combinations
    u8 lifetime;                 // Default lifetime (0 = infinite)
    
    // Custom behavior (optional)
    MaterialUpdateFn custom_update;
} MaterialDef;

// Helper macro for creating materials
#define MATERIAL_DEF(mat_id, mat_name, mat_state) \
    { .id = (mat_id), .name = (mat_name), .base_state = (mat_state) }

#endif
