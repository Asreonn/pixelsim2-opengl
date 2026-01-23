#include "world/grid_ops.h"
#include "world/world.h"
#include "materials/material_registry.h"
#include "core/math_utils.h"

static i32 grid_pick_dir(World* world, i32 x, i32 y) {
    u32 h = hash_position(x, y, world->tick);
    return (h & 1u) ? 1 : -1;
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
    return p && material_get_state(p->material_id) == STATE_LIQUID;
}

bool grid_is_neighbor_gas(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    MaterialState state = p ? material_get_state(p->material_id) : STATE_SOLID;
    return state == STATE_GAS || state == STATE_PLASMA;
}

bool grid_is_neighbor_solid(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    return p && material_get_state(p->material_id) == STATE_SOLID;
}

u16 grid_get_neighbor_material(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    return p ? p->material_id : MAT_AIR;
}

f32 grid_get_neighbor_density(World* world, i32 x, i32 y, Direction dir) {
    Pixel* p = grid_get_neighbor(world, x, y, dir);
    if (!p) return 0;
    return material_get_density(p->material_id);
}

bool grid_can_move_to(World* world, i32 x, i32 y, i32 to_x, i32 to_y) {
    Pixel* from = world_get_pixel(world, x, y);
    Pixel* to = world_get_pixel(world, to_x, to_y);
    
    if (!from || !to) return false;
    if (to->material_id == MAT_AIR) return true;
    
    // Check if can swap based on density
    f32 from_density = material_get_density(from->material_id);
    f32 to_density = material_get_density(to->material_id);
    MaterialState to_state = material_get_state(to->material_id);
    
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
    
    f32 from_density = material_get_density(from->material_id);
    f32 to_density = material_get_density(to->material_id);
    
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
    i32 dir = grid_pick_dir(world, x, y);
    if (grid_try_move(world, x, y, dir, 1)) return true;
    if (grid_try_move(world, x, y, -dir, 1)) return true;
    return false;
}

bool grid_try_move_down_diagonal_spread(World* world, i32 x, i32 y, i32 spread) {
    spread = clampi(spread, 1, MAX_SPREAD);
    i32 dir = grid_pick_dir(world, x, y);
    for (i32 i = 1; i <= spread; i++) {
        if (grid_try_move(world, x, y, dir * i, 1)) return true;
    }
    for (i32 i = 1; i <= spread; i++) {
        if (grid_try_move(world, x, y, -dir * i, 1)) return true;
    }
    return false;
}

bool grid_try_move_horizontal(World* world, i32 x, i32 y, i32 spread) {
    i32 dir = grid_pick_dir(world, x, y);
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

bool grid_try_move_up_diagonal(World* world, i32 x, i32 y) {
    i32 dir = grid_pick_dir(world, x, y);
    if (grid_try_move(world, x, y, dir, -1)) return true;
    if (grid_try_move(world, x, y, -dir, -1)) return true;
    return false;
}
