#include "world/grid_ops.h"
#include "world/world.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "core/math_utils.h"

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

// Calculate weight pressure from particles stacked above
i32 grid_get_weight_above(World* world, i32 x, i32 y, i32 max_height) {
    i32 weight = 0;
    for (i32 dy = 1; dy <= max_height; dy++) {
        Pixel* above = world_get_pixel(world, x, y - dy);
        if (!above) break;
        if (above->material_id == MAT_AIR) break;
        
        MaterialState state = MAT_CACHE_STATE(above->material_id);
        if (state == STATE_POWDER || state == STATE_LIQUID) {
            weight++;
        } else if (state == STATE_SOLID) {
            weight += 3;  // Solids add more pressure
            break;  // Solid blocks further pressure
        } else {
            break;  // Gas/plasma don't add weight
        }
    }
    return weight;
}

// Try to push particle sideways due to pressure from above
bool grid_try_pressure_push(World* world, i32 x, i32 y, i32 pressure) {
    if (pressure < 1) return false;
    
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel || pixel->material_id == MAT_AIR) return false;
    
    // Higher pressure = more likely to push and further distance
    i32 push_dist = clampi(pressure / 2, 1, 3);
    
    // Check if we're blocked below (otherwise we'd just fall)
    Pixel* below = world_get_pixel(world, x, y + 1);
    if (!below) return false;
    if (below->material_id == MAT_AIR) return false;  // Can fall, don't push
    
    // Also check diagonals - if either is open, prefer natural flow
    bool diag_left = grid_can_move_to(world, x, y, x - 1, y + 1);
    bool diag_right = grid_can_move_to(world, x, y, x + 1, y + 1);
    if (diag_left || diag_right) return false;  // Natural diagonal flow will handle it
    
    // We're truly stuck - try to push sideways
    i32 dir = grid_pick_dir_by_pressure(world, x, y);
    
    // Try pushing in preferred direction first
    for (i32 i = 1; i <= push_dist; i++) {
        if (grid_try_move(world, x, y, dir * i, 0)) return true;
    }
    // Try other direction
    for (i32 i = 1; i <= push_dist; i++) {
        if (grid_try_move(world, x, y, -dir * i, 0)) return true;
    }
    
    // Can't move horizontally - try pushing diagonally up (pile spreading)
    if (pressure >= 3) {
        if (grid_try_move(world, x, y, dir, -1)) return true;
        if (grid_try_move(world, x, y, -dir, -1)) return true;
    }
    
    return false;
}
