#ifndef PIXELSIM_PIXEL_H
#define PIXELSIM_PIXEL_H

#include "core/types.h"
#include "materials/material_ids.h"

// Pixel - dynamic state of each cell
typedef struct Pixel {
    u16 material_id;             // Which material
    
    // Dynamic properties
    f32 temperature;             // Kelvin
    f32 velocity_x;              // Horizontal velocity
    f32 velocity_y;              // Vertical velocity
    f32 pressure;                // Local pressure
    
    // State flags
    u8 lifetime;                 // Remaining lifetime (for fire etc.)
    u8 updated;                  // Updated this tick? (prevents double update)
    u8 color_seed;               // For color variation
    u8 flags;                    // Runtime flags
} Pixel;

// Pixel functions
void   pixel_init(Pixel* pixel, u16 material_id);
void   pixel_init_with_temp(Pixel* pixel, u16 material_id, f32 temperature);
void   pixel_reset(Pixel* pixel);
void   pixel_copy(Pixel* dest, const Pixel* src);
bool   pixel_is_empty(const Pixel* pixel);
bool   pixel_is_solid(const Pixel* pixel);
bool   pixel_is_liquid(const Pixel* pixel);
bool   pixel_is_gas(const Pixel* pixel);
bool   pixel_is_powder(const Pixel* pixel);
Color  pixel_get_color(const Pixel* pixel);

#endif
