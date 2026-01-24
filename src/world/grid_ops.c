#include "world/grid_ops.h"
#include "world/world.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "core/math_utils.h"
#include <math.h>

// Count empty/passable neighbors in a direction for pressure-based flow
static i32 grid_count_empty_in_direction(World* world, i32 x, i32 y, i32 dx, i32 max_dist) {
    i32 count = 0;
    for (i32 i = 1; i <= max_dist; i++) {
        Pixel* p = world_get_pixel(world, x + dx * i, y);
        if (!p) break;
        if (p->material_id == MAT_AIR) {
            count++;
        } else {
            break;
        }
    }
    return count;
}

// Pick direction based on pressure gradient (more empty space = lower pressure)
static i32 grid_pick_dir_by_pressure(World* world, i32 x, i32 y) {
    // Count empty space on each side
    i32 left_space = grid_count_empty_in_direction(world, x, y, -1, 3);
    i32 right_space = grid_count_empty_in_direction(world, x, y, 1, 3);
    
    // Also check diagonal down
    Pixel* down_left = world_get_pixel(world, x - 1, y + 1);
    Pixel* down_right = world_get_pixel(world, x + 1, y + 1);
    
    if (down_left && down_left->material_id == MAT_AIR) left_space += 2;
    if (down_right && down_right->material_id == MAT_AIR) right_space += 2;
    
    if (left_space > right_space) return -1;
    if (right_space > left_space) return 1;
    
    // Equal pressure - use position-based randomization with better distribution
    u32 h = hash_position(x, y, world->tick);
    // Use more bits for better distribution
    return ((h >> 3) & 1u) ? 1 : -1;
}

// Simple random direction with better hash
static i32 grid_pick_dir(World* world, i32 x, i32 y) {
    u32 h = hash_position(x, y, world->tick);
    // Use middle bits for better randomness
    return ((h >> 4) & 1u) ? 1 : -1;
}

Pixel* grid_get_neighbor(World* world, i32 x, i32 y, Direction dir) {
    return world_get_pixel(world, x + DIR_DX[dir], y + DIR_DY[dir]);
}

Pixel* grid_get_neighbor_offset(World* world, i32 x, i32 y, i32 dx, i32 dy) {
    return world_get_pixel(world, x + dx, y + dy);
}

bool grid_is_neighbor_empty(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    return p && p->material_id == MAT_AIR;
}

bool grid_is_neighbor_liquid(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    return p && MAT_CACHE_IS_LIQUID(p->material_id);
}

bool grid_is_neighbor_gas(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    return p && MAT_CACHE_IS_GAS(p->material_id);
}

bool grid_is_neighbor_solid(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    return p && MAT_CACHE_IS_SOLID(p->material_id);
}

u16 grid_get_neighbor_material(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    return p ? p->material_id : MAT_AIR;
}

f32 grid_get_neighbor_density(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    if (!p) return 0;
    return MAT_CACHE_DENSITY(p->material_id);
}

bool grid_can_move_to(World* world, i32 x, i32 y, i32 to_x, i32 to_y) {
    Pixel* from = world_get_pixel(world, x, y);
    Pixel* to = world_get_pixel(world, to_x, to_y);

    if (!from || !to) return false;
    if (to->material_id == MAT_AIR) return true;

    // Check if can swap based on density (use cached values)
    f32 from_density = MAT_CACHE_DENSITY(from->material_id);
    f32 to_density = MAT_CACHE_DENSITY(to->material_id);
    MaterialState to_state = MAT_CACHE_STATE(to->material_id);

    // === SICAKLIK BAZLI YOĞUNLUK KATMANLAŞMASI (KONVEKSİYON) ===
    // Aynı malzeme için sıcaklık farkı yoğunluğu etkiler
    // Sıcak sıvılar/gazlar daha hafif, yükselir
    if (from->material_id == to->material_id) {
        f32 temp_diff = from->temperature - to->temperature;
        // Her 10K sıcaklık farkı için ~%0.1 yoğunluk değişimi
        f32 temp_factor = 1.0f - (temp_diff * 0.0001f);
        from_density *= clampf(temp_factor, 0.9f, 1.1f);
    }

    // Can displace liquids and gases if denser
    if ((to_state == STATE_LIQUID || to_state == STATE_GAS || to_state == STATE_PLASMA) &&
        from_density > to_density) {
        return true;
    }

    return false;
}

bool grid_try_move(World* world, i32 x, i32 y, i32 dx, i32 dy) {
    i32 to_x = x + dx;
    i32 to_y = y + dy;
    
    if (!grid_can_move_to(world, x, y, to_x, to_y)) return false;
    
    Pixel* to = world_get_pixel(world, to_x, to_y);
    if (to->material_id == MAT_AIR) {
        world_move_pixel(world, x, y, to_x, to_y);
    } else {
        world_swap_pixels(world, x, y, to_x, to_y);
    }
    
    return true;
}

bool grid_try_swap_if_denser(World* world, i32 x, i32 y, i32 dx, i32 dy) {
    i32 to_x = x + dx;
    i32 to_y = y + dy;
    
    Pixel* from = world_get_pixel(world, x, y);
    Pixel* to = world_get_pixel(world, to_x, to_y);
    
    if (!from || !to) return false;
    
    f32 from_density = MAT_CACHE_DENSITY(from->material_id);
    f32 to_density = MAT_CACHE_DENSITY(to->material_id);
    
    if (from_density > to_density) {
        world_swap_pixels(world, x, y, to_x, to_y);
        return true;
    }
    
    return false;
}

i32 grid_count_neighbors_of_type(World* world, i32 x, i32 y, u16 material_id) {
    i32 count = 0;
    for (i32 i = 0; i < 8; i++) {
        Pixel* p = grid_get_neighbor(world, x, y, (Direction)i);
        if (p && p->material_id == material_id) count++;
    }
    return count;
}

f32 grid_get_average_neighbor_temp(World* world, i32 x, i32 y) {
    f32 sum = 0;
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        Pixel* p = grid_get_neighbor(world, x, y, (Direction)i);
        if (p) {
            sum += p->temperature;
            count++;
        }
    }
    return count > 0 ? sum / count : AMBIENT_TEMPERATURE;
}

f32 grid_get_average_neighbor_pressure(World* world, i32 x, i32 y) {
    f32 sum = 0;
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        Pixel* p = grid_get_neighbor(world, x, y, (Direction)i);
        if (p) {
            sum += p->pressure;
            count++;
        }
    }
    return count > 0 ? sum / count : AMBIENT_PRESSURE;
}

bool grid_try_move_down(World* world, i32 x, i32 y) {
    return grid_try_move(world, x, y, 0, 1);
}

bool grid_try_move_down_diagonal(World* world, i32 x, i32 y) {
    // Use pressure-based direction for more natural flow
    i32 dir = grid_pick_dir_by_pressure(world, x, y);
    if (grid_try_move(world, x, y, dir, 1)) return true;
    if (grid_try_move(world, x, y, -dir, 1)) return true;
    return false;
}

bool grid_try_move_down_diagonal_spread(World* world, i32 x, i32 y, i32 spread) {
    spread = clampi(spread, 1, MAX_SPREAD);
    i32 dir = grid_pick_dir_by_pressure(world, x, y);
    for (i32 i = 1; i <= spread; i++) {
        if (grid_try_move(world, x, y, dir * i, 1)) return true;
    }
    for (i32 i = 1; i <= spread; i++) {
        if (grid_try_move(world, x, y, -dir * i, 1)) return true;
    }
    return false;
}

bool grid_try_move_horizontal(World* world, i32 x, i32 y, i32 spread) {
    // Use pressure-based direction for natural flow toward empty space
    i32 dir = grid_pick_dir_by_pressure(world, x, y);
    for (i32 i = 1; i <= spread; i++) {
        if (grid_try_move(world, x, y, dir * i, 0)) return true;
    }
    for (i32 i = 1; i <= spread; i++) {
        if (grid_try_move(world, x, y, -dir * i, 0)) return true;
    }
    return false;
}

bool grid_try_move_up(World* world, i32 x, i32 y) {
    return grid_try_move(world, x, y, 0, -1);
}

// Pick direction for upward movement (gases) - prefer more empty space above
static i32 grid_pick_dir_up_by_pressure(World* world, i32 x, i32 y) {
    // Count empty space diagonally up on each side
    i32 left_space = 0, right_space = 0;
    
    for (i32 i = 1; i <= 3; i++) {
        Pixel* ul = world_get_pixel(world, x - i, y - i);
        Pixel* ur = world_get_pixel(world, x + i, y - i);
        if (ul && ul->material_id == MAT_AIR) left_space++;
        else break;
        if (ur && ur->material_id == MAT_AIR) right_space++;
        else break;
    }
    
    if (left_space > right_space) return -1;
    if (right_space > left_space) return 1;
    
    // Equal - use position-based randomization
    u32 h = hash_position(x, y, world->tick);
    return ((h >> 3) & 1u) ? 1 : -1;
}

bool grid_try_move_up_diagonal(World* world, i32 x, i32 y) {
    i32 dir = grid_pick_dir_up_by_pressure(world, x, y);
    if (grid_try_move(world, x, y, dir, -1)) return true;
    if (grid_try_move(world, x, y, -dir, -1)) return true;
    return false;
}

// Calculate weight/pressure from particles stacked above
// Returns accumulated weight considering density and velocity
f32 grid_get_stack_pressure(World* world, i32 x, i32 y, i32 max_height) {
    f32 pressure = 0.0f;
    
    for (i32 dy = 1; dy <= max_height; dy++) {
        Pixel* above = world_get_pixel(world, x, y - dy);
        if (!above) break;
        if (above->material_id == MAT_AIR) break;
        
        MaterialState state = MAT_CACHE_STATE(above->material_id);
        f32 density = MAT_CACHE_DENSITY(above->material_id);
        
        if (state == STATE_POWDER || state == STATE_LIQUID) {
            // Base pressure from density (normalized to ~1.0 for sand/water)
            f32 base_weight = density / 1500.0f;
            
            // Velocity adds momentum pressure - fast falling = more impact
            f32 velocity_bonus = fabsf(above->velocity_y) * 0.5f;
            
            pressure += base_weight + velocity_bonus;
        } else if (state == STATE_SOLID) {
            // Solids add massive pressure and block propagation
            pressure += (density / 1000.0f) * 3.0f;
            break;
        } else {
            break;  // Gas/plasma don't add weight
        }
    }
    
    return pressure;
}

// Check if there's an escape route (diagonal down or horizontal)
static bool grid_has_escape_route(World* world, i32 x, i32 y) {
    // Check diagonal down
    if (grid_can_move_to(world, x, y, x - 1, y + 1)) return true;
    if (grid_can_move_to(world, x, y, x + 1, y + 1)) return true;
    
    // Check horizontal
    Pixel* left = world_get_pixel(world, x - 1, y);
    Pixel* right = world_get_pixel(world, x + 1, y);
    if (left && left->material_id == MAT_AIR) return true;
    if (right && right->material_id == MAT_AIR) return true;
    
    return false;
}

// Try to push particle sideways due to pressure from above
// pressure: stack pressure value (typically 0-10+)
// viscosity: material viscosity (0 = flows easy, higher = resists)
bool grid_try_pressure_push(World* world, i32 x, i32 y, f32 pressure, f32 viscosity) {
    if (pressure < 0.5f) return false;

    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel || pixel->material_id == MAT_AIR) return false;

    // Friction and viscosity reduce effective pressure
    // High friction (sand=0.5, mud=0.75) greatly reduces spreading
    f32 friction = MAT_CACHE_FRICTION(pixel->material_id);
    f32 effective_pressure = pressure / (1.0f + viscosity * 0.5f + friction * 3.0f);
    if (effective_pressure < 0.3f) return false;

    // Check if we're blocked below (otherwise we'd just fall)
    Pixel* below = world_get_pixel(world, x, y + 1);
    if (!below) return false;
    if (below->material_id == MAT_AIR) return false;

    // If diagonals are open, let natural flow handle it (but only if pressure is low)
    if (effective_pressure < 2.0f && grid_has_escape_route(world, x, y)) {
        return false;
    }

    // Calculate push distance based on pressure and friction
    f32 dist_factor = 1.0f / (1.0f + friction * 2.0f);
    i32 push_dist = clampi((i32)(effective_pressure * dist_factor), 1, 4);
    
    // Pick direction based on where there's more space
    i32 dir = grid_pick_dir_by_pressure(world, x, y);
    
    // High pressure can push through other particles
    bool pushed = false;
    
    // First try direct horizontal movement
    for (i32 i = 1; i <= push_dist; i++) {
        if (grid_try_move(world, x, y, dir * i, 0)) {
            pushed = true;
            break;
        }
    }
    
    if (!pushed) {
        // Try other direction
        for (i32 i = 1; i <= push_dist; i++) {
            if (grid_try_move(world, x, y, -dir * i, 0)) {
                pushed = true;
                break;
            }
        }
    }
    
    // High pressure: try diagonal down even if blocked directly below
    if (!pushed && effective_pressure >= 1.5f) {
        if (grid_try_move(world, x, y, dir, 1)) pushed = true;
        else if (grid_try_move(world, x, y, -dir, 1)) pushed = true;
    }
    
    // Very high pressure: can push diagonally up (pile spreading/avalanche)
    if (!pushed && effective_pressure >= 3.0f) {
        if (grid_try_move(world, x, y, dir, -1)) pushed = true;
        else if (grid_try_move(world, x, y, -dir, -1)) pushed = true;
    }
    
    // Transfer some velocity from pressure
    if (pushed) {
        Pixel* moved = world_get_pixel(world, x + dir, y);  // approximate new pos
        if (!moved) moved = world_get_pixel(world, x - dir, y);
        if (moved && moved->material_id != MAT_AIR) {
            moved->velocity_x += dir * effective_pressure * 0.3f;
        }
    }
    
    return pushed;
}

// Legacy wrapper for compatibility
i32 grid_get_weight_above(World* world, i32 x, i32 y, i32 max_height) {
    return (i32)grid_get_stack_pressure(world, x, y, max_height);
}

// Propagate pressure upward - push particles above to spread out
// This creates avalanche/cascade effect
bool grid_propagate_pressure_up(World* world, i32 x, i32 y, i32 max_height) {
    bool any_moved = false;
    
    // Start from position and go up, trying to push each particle sideways
    for (i32 dy = 1; dy <= max_height; dy++) {
        Pixel* above = world_get_pixel(world, x, y - dy);
        if (!above) break;
        if (above->material_id == MAT_AIR) break;
        
        MaterialState state = MAT_CACHE_STATE(above->material_id);
        if (state != STATE_POWDER && state != STATE_LIQUID) break;
        
        // Try to push this particle sideways
        f32 viscosity = MAT_CACHE_VISCOSITY(above->material_id);
        
        // Calculate pressure at this height (decreases as we go up)
        f32 pressure = (f32)(max_height - dy + 1) * 0.8f;
        
        if (grid_try_pressure_push(world, x, y - dy, pressure, viscosity)) {
            any_moved = true;
            // After moving, the column above might have shifted
            // Check if there's still something at this position
            Pixel* check = world_get_pixel(world, x, y - dy);
            if (!check || check->material_id == MAT_AIR) {
                // Gap created, stop propagation
                break;
            }
        }
    }
    
    return any_moved;
}
