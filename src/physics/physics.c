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
#include "materials/material_cache.h"
#include "materials/interactions.h"
#include "core/math_utils.h"
#include "core/thread_pool.h"
#include <math.h>

// Work item for parallel chunk processing
typedef struct ChunkWorkItem {
    World* world;
    Chunk* chunk;
    f32 dt;
    bool gas_pass;
} ChunkWorkItem;

// Pool of work items to avoid allocation during simulation
#define MAX_WORK_ITEMS (CHUNKS_X * CHUNKS_Y)
static ChunkWorkItem g_work_items[MAX_WORK_ITEMS];
static i32 g_work_item_index = 0;

static inline bool physics_is_gas(MaterialState state) {
    return state == STATE_GAS || state == STATE_PLASMA;
}

// Forward declaration
static void physics_update_chunk_pass(World* world, Chunk* chunk, f32 dt, bool gas_pass);

// Worker function for parallel chunk processing
static void physics_chunk_worker(void* arg) {
    ChunkWorkItem* item = (ChunkWorkItem*)arg;
    physics_update_chunk_pass(item->world, item->chunk, item->dt, item->gas_pass);
}

// Allocate work item from pool
static ChunkWorkItem* physics_alloc_work_item(World* world, Chunk* chunk, f32 dt, bool gas_pass) {
    ChunkWorkItem* item = &g_work_items[g_work_item_index++];
    if (g_work_item_index >= MAX_WORK_ITEMS) g_work_item_index = 0;
    item->world = world;
    item->chunk = chunk;
    item->dt = dt;
    item->gas_pass = gas_pass;
    return item;
}

static void physics_reset_gas_updates(World* world) {
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (chunk_is_sleeping(chunk)) continue;
            
            // Grid scan to reset gas update flags
            for (i32 ly = 0; ly < CHUNK_SIZE; ly++) {
                for (i32 lx = 0; lx < CHUNK_SIZE; lx++) {
                    Pixel* pixel = chunk_get_pixel_fast(chunk, lx, ly);
                    if (pixel->material_id == MAT_AIR) continue;
                    if (MAT_CACHE_IS_GAS(pixel->material_id)) {
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

    // ALWAYS use ordered grid scan for correct falling physics
    // Bottom-to-top for non-gas (falling), top-to-bottom for gas (rising)
    i32 ly_start = gas_pass ? 0 : CHUNK_SIZE - 1;
    i32 ly_end = gas_pass ? CHUNK_SIZE : -1;
    i32 ly_step = gas_pass ? 1 : -1;

    for (i32 ly = ly_start; ly != ly_end; ly += ly_step) {
        // Alternate horizontal direction each row to prevent bias
        i32 x_start = ((ly + world->tick) % 2 == 0) ? 0 : CHUNK_SIZE - 1;
        i32 x_end = ((ly + world->tick) % 2 == 0) ? CHUNK_SIZE : -1;
        i32 x_step = ((ly + world->tick) % 2 == 0) ? 1 : -1;

        for (i32 lx = x_start; lx != x_end; lx += x_step) {
            Pixel* pixel = chunk_get_pixel_fast(chunk, lx, ly);
            
            // Quick skip for empty/updated pixels
            if (pixel->material_id == MAT_AIR || pixel->updated) continue;

            MaterialState state = MAT_CACHE_STATE(pixel->material_id);
            if (physics_is_gas(state) != gas_pass) continue;

            i32 x = base_x + lx;
            i32 y = base_y + ly;
            physics_update_pixel(world, x, y, dt);
        }
    }
}

// Serial (single-threaded) physics update
static void physics_update_serial(World* world, f32 dt) {
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
}

// Parallel physics update using checkerboard pattern with row-by-row ordering
// Processes chunks that don't share edges in parallel, row by row for correct physics
static void physics_update_parallel(World* world, f32 dt) {
    ThreadPool* pool = world->thread_pool;
    
    // Phase 1: Non-gas pass (bottom to top, row by row)
    for (i32 cy = CHUNKS_Y - 1; cy >= 0; cy--) {
        // Process even-column chunks for this row in parallel
        for (i32 cx = 0; cx < CHUNKS_X; cx += 2) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (!chunk_is_sleeping(chunk)) {
                ChunkWorkItem* item = physics_alloc_work_item(world, chunk, dt, false);
                thread_pool_submit(pool, physics_chunk_worker, item);
            }
        }
        thread_pool_wait(pool);
        
        // Process odd-column chunks for this row in parallel
        for (i32 cx = 1; cx < CHUNKS_X; cx += 2) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (!chunk_is_sleeping(chunk)) {
                ChunkWorkItem* item = physics_alloc_work_item(world, chunk, dt, false);
                thread_pool_submit(pool, physics_chunk_worker, item);
            }
        }
        thread_pool_wait(pool);
    }
    
    physics_reset_gas_updates(world);
    
    // Phase 2: Gas pass (top to bottom for rising gases, row by row)
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        // Even columns
        for (i32 cx = 0; cx < CHUNKS_X; cx += 2) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (!chunk_is_sleeping(chunk)) {
                ChunkWorkItem* item = physics_alloc_work_item(world, chunk, dt, true);
                thread_pool_submit(pool, physics_chunk_worker, item);
            }
        }
        thread_pool_wait(pool);
        
        // Odd columns
        for (i32 cx = 1; cx < CHUNKS_X; cx += 2) {
            Chunk* chunk = &world->chunks[cy][cx];
            if (!chunk_is_sleeping(chunk)) {
                ChunkWorkItem* item = physics_alloc_work_item(world, chunk, dt, true);
                thread_pool_submit(pool, physics_chunk_worker, item);
            }
        }
        thread_pool_wait(pool);
    }
}

// Wake neighbor chunks if pixels are at edges (prevents boundary sticking)
static void physics_wake_neighbor_chunks(World* world) {
    for (i32 cy = 0; cy < CHUNKS_Y; cy++) {
        for (i32 cx = 0; cx < CHUNKS_X; cx++) {
            Chunk* chunk = &world->chunks[cy][cx];
            
            // Skip empty/sleeping chunks with no recent activity
            if (chunk->active_count == 0 && chunk->changes_this_tick == 0) continue;
            
            // Check bottom edge - wake chunk below for falling pixels
            if (cy < CHUNKS_Y - 1 && chunk_has_pixel_at_bottom_edge(chunk)) {
                Chunk* below = &world->chunks[cy + 1][cx];
                chunk_wake(below);
            }
            
            // Check top edge - wake chunk above for rising gases
            if (cy > 0 && chunk_has_pixel_at_top_edge(chunk)) {
                Chunk* above = &world->chunks[cy - 1][cx];
                chunk_wake(above);
            }
            
            // Check left edge - wake chunk to the left for horizontal spread
            if (cx > 0 && chunk_has_pixel_at_left_edge(chunk)) {
                Chunk* left = &world->chunks[cy][cx - 1];
                chunk_wake(left);
            }
            
            // Check right edge - wake chunk to the right for horizontal spread
            if (cx < CHUNKS_X - 1 && chunk_has_pixel_at_right_edge(chunk)) {
                Chunk* right = &world->chunks[cy][cx + 1];
                chunk_wake(right);
            }
        }
    }
}

void physics_update(World* world, f32 dt) {
    // PHASE 1: Pre-calculate stack pressure for all pixels
    // This must happen BEFORE movement so pixels know their pressure
    pressure_calculate_all_stacks(world);
    
    // PHASE 2: Movement and physics update
    if (world->use_parallel && world->thread_pool) {
        physics_update_parallel(world, dt);
    } else {
        physics_update_serial(world, dt);
    }
    
    // Wake neighbor chunks before sleep check
    physics_wake_neighbor_chunks(world);
    
    // Update chunk states (always serial - lightweight)
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
    u16 mat_id = pixel->material_id;

    // Use cached material state for fast lookup
    MaterialState state = MAT_CACHE_STATE(mat_id);

    // === KATILAR İÇİN ERKEN ÇIKIŞ OPTİMİZASYONU ===
    // Stabil katılar (ambient sıcaklık, düşük basınç) çoğu işlemi atlar
    if (state == STATE_SOLID) {
        f32 temp_diff = fabsf(pixel->temperature - world->ambient_temperature);
        if (temp_diff < 5.0f && pixel->pressure < AMBIENT_PRESSURE + 1000.0f) {
            // Her 10 tick'te bir sıcaklık kontrolü (10x dt ile)
            if ((world->tick % 10) == 0) {
                temperature_update(world, x, y, dt * 10.0f);
            }
            return;  // Çoğu işlemi atla - %80+ katı piksel için büyük tasarruf
        }
    }

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
    
    // 4. Custom material update (need full MaterialDef for function pointer)
    MaterialDef* mat = material_get(mat_id);
    if (mat && mat->custom_update) {
        mat->custom_update(pixel, world, x, y);
    }
    
    // 5. Apply gravity
    gravity_apply(pixel, world, x, y, dt);
    
    // 6. Apply movement based on state
    movement_update(world, x, y);
    
    // 7. Pressure update for gases (use cached state)
    if (state == STATE_GAS || state == STATE_PLASMA) {
        pressure_apply_forces(world, x, y, dt);
    }
}
