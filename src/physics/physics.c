#include "physics/physics.h"
#include "physics/gravity.h"
#include "physics/temperature.h"
#include "physics/pressure.h"
#include "physics/movement.h"
#include "physics/phase_transition.h"
#include "world/world.h"
#include "world/chunk.h"
#include "world/grid_ops.h"
#include "materials/material_registry.h"
#include "materials/interactions.h"
#include "core/math_utils.h"

static bool physics_is_gas(MaterialState state) {
    return state == STATE_GAS || state == STATE_PLASMA;
}

static void physics_reset_gas_updates(World* world) {
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            Chunk* chunk = &world->chunks[cy][cx];
            for (i32 ly = 0; ly < CHUNK_SIZE; ly++) {
                for (i32 lx = 0; lx < CHUNK_SIZE; lx++) {
                    Pixel* pixel = chunk_get_pixel(chunk, lx, ly);
                    if (!pixel || pixel->material_id == MAT_AIR) continue;
                    if (physics_is_gas(material_get_state(pixel->material_id))) {
                        pixel->updated = 0;
                    }
                }
            }
        }
    }
}

static void physics_update_chunk_pass(World* world, Chunk* chunk, f32 dt, bool gas_pass) {
    i32 base_x = chunk->chunk_x * CHUNK_SIZE;
    i32 base_y = chunk->chunk_y * CHUNK_SIZE;

    i32 ly_start = gas_pass ? 0 : CHUNK_SIZE - 1;
    i32 ly_end = gas_pass ? CHUNK_SIZE : -1;
    i32 ly_step = gas_pass ? 1 : -1;

    for (i32 ly = ly_start; ly != ly_end; ly += ly_step) {
        i32 x_start = ((ly + world->tick) % 2 == 0) ? 0 : CHUNK_SIZE - 1;
        i32 x_end = ((ly + world->tick) % 2 == 0) ? CHUNK_SIZE : -1;
        i32 x_step = ((ly + world->tick) % 2 == 0) ? 1 : -1;

        for (i32 lx = x_start; lx != x_end; lx += x_step) {
            i32 x = base_x + lx;
            i32 y = base_y + ly;

            Pixel* pixel = chunk_get_pixel(chunk, lx, ly);
            if (!pixel || pixel->updated || pixel->material_id == MAT_AIR) continue;

            MaterialState state = material_get_state(pixel->material_id);
            if (physics_is_gas(state) != gas_pass) continue;

            physics_update_pixel(world, x, y, dt);
        }
    }
}

void physics_update(World* world, f32 dt) {
    for (i32 cy = CHUNKS_Y - 1; cy >= 0; cy--) {
        i32 cx_start = ((cy + world->tick) % 2 == 0) ? 0 : CHUNKS_X - 1;
        i32 cx_end = ((cy + world->tick) % 2 == 0) ? CHUNKS_X : -1;
        i32 cx_step = ((cy + world->tick) % 2 == 0) ? 1 : -1;
        for (i32 cx = cx_start; cx != cx_end; cx += cx_step) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (!chunk_is_sleeping(chunk)) {
                physics_update_chunk_pass(world, chunk, dt, false);
            }
        }

        if (cy == 0) break;
    }

    physics_reset_gas_updates(world);

    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        i32 cx_start = ((cy + world->tick) % 2 == 0) ? 0 : CHUNKS_X - 1;
        i32 cx_end = ((cy + world->tick) % 2 == 0) ? CHUNKS_X : -1;
        i32 cx_step = ((cy + world->tick) % 2 == 0) ? 1 : -1;
        for (i32 cx = cx_start; cx != cx_end; cx += cx_step) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (!chunk_is_sleeping(chunk)) {
                physics_update_chunk_pass(world, chunk, dt, true);
            }
        }
    }

    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            Chunk* chunk = &world->chunks[cy][cx];
            chunk_update_active_state(chunk);
            chunk_try_sleep(chunk);
        }
    }
}

void physics_update_chunk(World* world, Chunk* chunk, f32 dt) {
    physics_update_chunk_pass(world, chunk, dt, false);
    physics_update_chunk_pass(world, chunk, dt, true);
    chunk_update_active_state(chunk);
}

void physics_update_pixel(World* world, i32 x, i32 y, f32 dt) {
    Pixel* pixel = world_get_pixel(world, x, y);
    if (!pixel || pixel->updated) return;
    
    pixel->updated = 1;
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return;
    
    // 1. Temperature update
    temperature_update(world, x, y, dt);
    
    // 2. Phase transitions
    if (phase_check_transitions(world, x, y)) {
        return; // Pixel transformed, skip rest
    }
    
    // 3. Check material interactions with neighbors
    for (i32 i = 0; i < 4; i++) {
        interaction_process(world, x, y, x + DIR_DX[i], y + DIR_DY[i]);
    }
    
    // 4. Custom material update
    if (mat->custom_update) {
        mat->custom_update(pixel, world, x, y);
    }
    
    // 5. Apply gravity
    gravity_apply(pixel, world, x, y, dt);
    
    // 6. Apply movement based on state
    movement_update(world, x, y);
    
    // 7. Pressure update for gases
    if (mat->base_state == STATE_GAS || mat->base_state == STATE_PLASMA) {
        pressure_apply_forces(world, x, y, dt);
    }
}
