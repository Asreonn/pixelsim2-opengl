#include "materials/interactions.h"
#include "world/world.h"
#include "world/pixel.h"
#include "materials/material_registry.h"
#include "core/config.h"
#include "core/math_utils.h"

#define MUD_SPREAD_RADIUS 8

static MaterialInteraction interactions[MAX_INTERACTIONS];
static u32 interaction_count = 0;

void interactions_init(void) {
    interaction_count = 0;
    
    // Water + Sand = Mud
    interaction_add(MAT_WATER, MAT_SAND, MAT_MUD, MAT_AIR, INTERACT_COMBINE, 1.0f);
    
    // Water + Fire = Steam
    interaction_add(MAT_WATER, MAT_FIRE, MAT_STEAM, MAT_AIR, INTERACT_REACT, 0.5f);
    
    // Fire + Ice = Water/Steam
    interaction_add(MAT_FIRE, MAT_ICE, MAT_STEAM, MAT_WATER, INTERACT_REACT, 0.3f);
}

void interactions_shutdown(void) {
    interaction_count = 0;
}

bool interaction_register(const MaterialInteraction* interaction) {
    if (!interaction || interaction_count >= MAX_INTERACTIONS) return false;
    
    interactions[interaction_count++] = *interaction;
    return true;
}

bool interaction_add(u16 mat_a, u16 mat_b, u16 result_a, u16 result_b, 
                     InteractionType type, f32 probability) {
    MaterialInteraction inter = {
        .material_a = mat_a,
        .material_b = mat_b,
        .result_a = result_a,
        .result_b = result_b,
        .type = type,
        .probability = probability,
        .min_temperature = 0
    };
    return interaction_register(&inter);
}

const MaterialInteraction* interaction_find(u16 mat_a, u16 mat_b) {
    for (u32 i = 0; i < interaction_count; i++) {
        MaterialInteraction* inter = &interactions[i];
        if ((inter->material_a == mat_a && inter->material_b == mat_b) ||
            (inter->material_a == mat_b && inter->material_b == mat_a)) {
            return inter;
        }
    }
    return NULL;
}

static bool interaction_should_apply(World* world, i32 x1, i32 y1, i32 x2, i32 y2,
                                     const MaterialInteraction* inter, f32 temperature) {
    if (!inter) return false;
    if (temperature < inter->min_temperature) return false;
    if (inter->probability >= 1.0f) return true;
    if (inter->probability <= 0.0f) return false;

    u32 h = hash_position(x1 + x2, y1 + y2, world->tick);
    f32 sample = (f32)(h & 0xFFFFu) / 65535.0f;
    return sample < inter->probability;
}

static bool interaction_apply_mud(World* world, i32 x, i32 y, f32 temp) {
    const i32 radius = MUD_SPREAD_RADIUS;

    for (i32 r = 0; r <= radius; r++) {
        i32 r_sq = r * r;
        bool found = false;
        i32 best_x = 0;
        i32 best_y = 0;
        u32 best_hash = 0xFFFFFFFFu;

        for (i32 dy = -r; dy <= r; dy++) {
            for (i32 dx = -r; dx <= r; dx++) {
                if (dx * dx + dy * dy > r_sq) continue;

                i32 px = x + dx;
                i32 py = y + dy;
                Pixel* target = world_get_pixel(world, px, py);
                if (!target || target->material_id != MAT_SAND) continue;

                u32 h = hash_position(px, py, 0);
                if (!found || h < best_hash) {
                    found = true;
                    best_hash = h;
                    best_x = px;
                    best_y = py;
                }
            }
        }

        if (found) {
            Pixel* target = world_get_pixel(world, best_x, best_y);
            if (target) {
                pixel_init_with_temp(target, MAT_MUD, temp);
                world_wake_chunk_at(world, best_x, best_y);
                return true;
            }
        }
    }

    return false;
}

bool interaction_process(World* world, i32 x1, i32 y1, i32 x2, i32 y2) {
    Pixel* p1 = world_get_pixel(world, x1, y1);
    Pixel* p2 = world_get_pixel(world, x2, y2);
    
    if (!p1 || !p2) return false;
    if (p1->material_id == MAT_AIR || p2->material_id == MAT_AIR) return false;
    
    f32 avg_temp = (p1->temperature + p2->temperature) * 0.5f;

    if ((p1->material_id == MAT_WATER && p2->material_id == MAT_SAND) ||
        (p1->material_id == MAT_SAND && p2->material_id == MAT_WATER)) {
        Pixel* water = (p1->material_id == MAT_WATER) ? p1 : p2;
        i32 water_x = (p1->material_id == MAT_WATER) ? x1 : x2;
        i32 water_y = (p1->material_id == MAT_WATER) ? y1 : y2;
        i32 sand_x = (p1->material_id == MAT_SAND) ? x1 : x2;
        i32 sand_y = (p1->material_id == MAT_SAND) ? y1 : y2;

        if (interaction_apply_mud(world, sand_x, sand_y, avg_temp)) {
            pixel_init_with_temp(water, MAT_AIR, avg_temp);
            world_wake_chunk_at(world, water_x, water_y);
            return true;
        }

        return false;
    }

    if ((p1->material_id == MAT_WATER && p2->material_id == MAT_MUD) ||
        (p1->material_id == MAT_MUD && p2->material_id == MAT_WATER)) {
        Pixel* water = (p1->material_id == MAT_WATER) ? p1 : p2;
        i32 water_x = (p1->material_id == MAT_WATER) ? x1 : x2;
        i32 water_y = (p1->material_id == MAT_WATER) ? y1 : y2;
        i32 mud_x = (p1->material_id == MAT_MUD) ? x1 : x2;
        i32 mud_y = (p1->material_id == MAT_MUD) ? y1 : y2;

        if (interaction_apply_mud(world, mud_x, mud_y, avg_temp)) {
            pixel_init_with_temp(water, MAT_AIR, avg_temp);
            world_wake_chunk_at(world, water_x, water_y);
            return true;
        }

        return false;
    }

    const MaterialInteraction* inter = interaction_find(p1->material_id, p2->material_id);
    if (!inter) return false;

    if (!interaction_should_apply(world, x1, y1, x2, y2, inter, avg_temp)) return false;
    
    // Determine which pixel is which material
    bool swapped = (p1->material_id == inter->material_b);
    Pixel* pa = swapped ? p2 : p1;
    Pixel* pb = swapped ? p1 : p2;
    i32 xa = swapped ? x2 : x1;
    i32 ya = swapped ? y2 : y1;
    i32 xb = swapped ? x1 : x2;
    i32 yb = swapped ? y1 : y2;
    
    switch (inter->type) {
        case INTERACT_COMBINE:
            // A becomes result_a, B becomes result_b (usually air)
            pixel_init_with_temp(pa, inter->result_a, avg_temp);
            pixel_init_with_temp(pb, inter->result_b, avg_temp);
            world_wake_chunk_at(world, xa, ya);
            world_wake_chunk_at(world, xb, yb);
            break;
            
        case INTERACT_REACT:
            pixel_init_with_temp(pa, inter->result_a, avg_temp);
            pixel_init_with_temp(pb, inter->result_b, avg_temp);
            world_wake_chunk_at(world, xa, ya);
            world_wake_chunk_at(world, xb, yb);
            break;
            
        case INTERACT_DISSOLVE:
            // A dissolves into B
            pixel_init_with_temp(pa, inter->result_b, avg_temp);
            world_wake_chunk_at(world, xa, ya);
            break;
            
        case INTERACT_SPREAD:
            // A spreads to B (like fire)
            pixel_init_with_temp(pb, inter->result_a, avg_temp);
            world_wake_chunk_at(world, xb, yb);
            break;
            
        default:
            return false;
    }
    
    return true;
}
