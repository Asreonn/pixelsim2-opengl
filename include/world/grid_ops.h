#ifndef PIXELSIM_GRID_OPS_H
#define PIXELSIM_GRID_OPS_H

#include "core/types.h"
#include "world/world.h"

// Neighbor access
Pixel* grid_get_neighbor(World* world, i32 x, i32 y, Direction dir);
Pixel* grid_get_neighbor_offset(World* world, i32 x, i32 y, i32 dx, i32 dy);

// Neighbor queries
bool   grid_is_neighbor_empty(World* world, i32 x, i32 y, Direction dir);
bool   grid_is_neighbor_liquid(World* world, i32 x, i32 y, Direction dir);
bool   grid_is_neighbor_gas(World* world, i32 x, i32 y, Direction dir);
bool   grid_is_neighbor_solid(World* world, i32 x, i32 y, Direction dir);
u16    grid_get_neighbor_material(World* world, i32 x, i32 y, Direction dir);
f32    grid_get_neighbor_density(World* world, i32 x, i32 y, Direction dir);

// Movement helpers
bool   grid_can_move_to(World* world, i32 x, i32 y, i32 to_x, i32 to_y);
bool   grid_try_move(World* world, i32 x, i32 y, i32 dx, i32 dy);
bool   grid_try_swap_if_denser(World* world, i32 x, i32 y, i32 dx, i32 dy);

// Area operations
i32    grid_count_neighbors_of_type(World* world, i32 x, i32 y, u16 material_id);
f32    grid_get_average_neighbor_temp(World* world, i32 x, i32 y);
f32    grid_get_average_neighbor_pressure(World* world, i32 x, i32 y);

// Directional movement helpers
bool   grid_try_move_down(World* world, i32 x, i32 y);
bool   grid_try_move_down_diagonal(World* world, i32 x, i32 y);
bool   grid_try_move_down_diagonal_spread(World* world, i32 x, i32 y, i32 spread);
bool   grid_try_move_horizontal(World* world, i32 x, i32 y, i32 spread);
bool   grid_try_move_up(World* world, i32 x, i32 y);
bool   grid_try_move_up_diagonal(World* world, i32 x, i32 y);

// Pressure-based movement
f32    grid_get_stack_pressure(World* world, i32 x, i32 y, i32 max_height);
i32    grid_get_weight_above(World* world, i32 x, i32 y, i32 max_height);
bool   grid_try_pressure_push(World* world, i32 x, i32 y, f32 pressure, f32 viscosity);

#endif
