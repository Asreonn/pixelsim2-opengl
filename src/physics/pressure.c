#include "physics/pressure.h"
#include <math.h>
#include "world/world.h"
#include "world/chunk.h"
#include "world/pixel.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "core/config.h"
#include "core/math_utils.h"

f32 pressure_calculate_hydrostatic(World* world, i32 x, i32 y) {
    // P = P0 + rho * g * h
    // Count liquid pixels above to determine depth
    f32 pressure = AMBIENT_PRESSURE;
    
    for (i32 dy = -1; dy >= -100; dy--) {
        Pixel* above = world_get_pixel(world, x, y + dy);
        if (!above) break;
        
        MaterialDef* mat = material_get(above->material_id);
        if (!mat) break;
        
        if (mat->base_state == STATE_LIQUID) {
            pressure += mat->density * world->gravity * 1.0f; // 1 pixel height
        } else if (mat->base_state != STATE_GAS && mat->base_state != STATE_PLASMA) {
            break; // Hit solid
        }
    }
    
    return pressure;
}

void pressure_update_gas(Pixel* pixel, f32 local_density, f32 dt) {
    if (!pixel) return;
    
    // Simplified ideal gas: P proportional to T
    // P = density * R * T (simplified)
    f32 gas_constant = 0.01f;
    pixel->pressure = local_density * gas_constant * pixel->temperature;
    
    // Pressure equilibration with neighbors happens in transfer function
    (void)dt;
}

f32 pressure_calculate_local(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return AMBIENT_PRESSURE;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return AMBIENT_PRESSURE;
    
    if (mat->base_state == STATE_LIQUID) {
        return pressure_calculate_hydrostatic(world, x, y);
    } else if (mat->base_state == STATE_GAS || mat->base_state == STATE_PLASMA) {
        return pixel->pressure;
    }
    
    return AMBIENT_PRESSURE;
}

void pressure_apply_forces(World* world, i32 x, i32 y, f32 dt) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return;
    
    f32 local_pressure = pixel->pressure;
    
    // Check pressure gradient with neighbors
    for (i32 i = 0; i < 4; i++) {
        Pixel* neighbor = grid_get_neighbor(world, x, y, (Direction)i);
        if (!neighbor) continue;
        
        f32 pressure_diff = local_pressure - neighbor->pressure;
        
        // Push away from high pressure
        if (pressure_diff > 0) {
            f32 force = pressure_diff * PRESSURE_TRANSFER_RATE * dt;
            pixel->velocity_x -= DIR_DX[i] * force * 0.01f;
            pixel->velocity_y -= DIR_DY[i] * force * 0.01f;
        }
    }
}

bool pressure_check_explosion(World* world, i32 x, i32 y) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel) return false;
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return false;
    
    // Check if material is explosive and pressure exceeds threshold
    if ((mat->flags & MAT_FLAG_EXPLOSIVE) && pixel->pressure > EXPLOSION_THRESHOLD) {
        return true;
    }
    
    return false;
}

void pressure_create_explosion(World* world, i32 x, i32 y, f32 power) {
    // Calculate radius based on power
    i32 radius = (i32)(sqrtf(power) * 0.1f);
    radius = clampi(radius, 1, 20);
    
    for (i32 dy = -radius; dy <= radius; dy++) {
        for (i32 dx = -radius; dx <= radius; dx++) {
            i32 dist_sq = dx * dx + dy * dy;
            if (dist_sq > radius * radius) continue;
            
            Pixel* p = world_get_pixel(world, x + dx, y + dy);
            if (!p) continue;
            
            MaterialDef* mat = material_get(p->material_id);
            if (!mat) continue;
            
            // Destroy non-indestructible materials
            if (mat->base_state != STATE_SOLID || dist_sq < radius * radius / 4) {
                pixel_reset(p);
            }
            
            // Add heat
            p->temperature += power / (1.0f + dist_sq);
            
            // Add outward velocity
            if (dist_sq > 0) {
                f32 dist = sqrtf((f32)dist_sq);
                p->velocity_x += (dx / dist) * power * 0.01f;
                p->velocity_y += (dy / dist) * power * 0.01f;
            }
        }
    }
}

void pressure_transfer(Pixel* p1, Pixel* p2, f32 rate, f32 dt) {
    if (!p1 || !p2) return;
    
    f32 diff = p1->pressure - p2->pressure;
    f32 transfer = diff * rate * dt;
    
    p1->pressure -= transfer;
    p2->pressure += transfer;
}

// Pre-calculate stack pressure for all pixels in a chunk
// This runs BEFORE movement update so each pixel knows its pressure
void pressure_calculate_chunk_stack(World* world, i32 chunk_x, i32 chunk_y) {
    i32 base_x = chunk_x * CHUNK_SIZE;
    i32 base_y = chunk_y * CHUNK_SIZE;
    
    // Process each column in the chunk
    for (i32 lx = 0; lx < CHUNK_SIZE; lx++) {
        i32 x = base_x + lx;
        
        // Scan from top to bottom, accumulating weight
        f32 accumulated_weight = 0.0f;
        
        // First, check pixels above this chunk for incoming pressure
        for (i32 check_y = base_y - 1; check_y >= 0; check_y--) {
            Pixel* above = world_get_pixel(world, x, check_y);
            if (!above || above->material_id == MAT_AIR) break;
            
            MaterialState state = MAT_CACHE_STATE(above->material_id);
            if (state == STATE_SOLID) {
                accumulated_weight += MAT_CACHE_DENSITY(above->material_id) / 500.0f;
                break;
            } else if (state == STATE_POWDER || state == STATE_LIQUID) {
                f32 density = MAT_CACHE_DENSITY(above->material_id);
                accumulated_weight += density / 1500.0f;
                accumulated_weight += fabsf(above->velocity_y) * 0.3f;
            } else {
                break;
            }
        }
        
        // Now process this chunk's column top to bottom
        for (i32 ly = 0; ly < CHUNK_SIZE; ly++) {
            i32 y = base_y + ly;
            Pixel* pixel = world_get_pixel(world, x, y);
            if (!pixel) continue;
            
            if (pixel->material_id == MAT_AIR) {
                accumulated_weight = 0.0f;
                continue;
            }
            
            MaterialState state = MAT_CACHE_STATE(pixel->material_id);
            
            if (state == STATE_POWDER || state == STATE_LIQUID) {
                // Store accumulated pressure from above
                pixel->pressure = AMBIENT_PRESSURE + accumulated_weight * 15000.0f;
                
                // Add this pixel's weight for pixels below
                f32 density = MAT_CACHE_DENSITY(pixel->material_id);
                accumulated_weight += density / 1500.0f;
                accumulated_weight += fabsf(pixel->velocity_y) * 0.3f;
            } else if (state == STATE_SOLID) {
                // Solids block pressure but add their weight
                pixel->pressure = AMBIENT_PRESSURE;
                accumulated_weight += MAT_CACHE_DENSITY(pixel->material_id) / 500.0f;
            } else {
                // Gas/plasma reset pressure accumulation
                pixel->pressure = AMBIENT_PRESSURE;
                accumulated_weight = 0.0f;
            }
        }
    }
}

// Calculate stack pressure for entire world (call before movement update)
void pressure_calculate_all_stacks(World* world) {
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (chunk_is_sleeping(chunk)) continue;
            pressure_calculate_chunk_stack(world, cx, cy);
        }
    }
}
