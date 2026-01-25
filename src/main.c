#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/types.h"
#include "core/config.h"
#include "core/math_utils.h"
#include "world/world.h"
#include "materials/material_registry.h"
#include "materials/material_cache.h"
#include "materials/interactions.h"
#include "render/renderer.h"
#include "input/input.h"
#include "input/brush.h"

int main(void) {
    printf("PixelSim2 - Deterministic Physics Simulator\n");
    printf("============================================\n");
    printf("Controls:\n");
    printf("  Left Click  - Paint material\n");
    printf("  Right Click - Erase\n");
    printf("  Scroll      - Brush size\n");
    printf("  1-8         - Select material\n");
    printf("  9           - Air\n");
    printf("  Space       - Pause/Resume\n");
    printf("  R           - Reset world\n");
    printf("  T           - Temperature view\n");
    printf("  P           - Pressure view\n");
    printf("============================================\n\n");
    
    // Initialize random seed
    rand_seed((u32)time(NULL));
    
    // Initialize materials
    printf("Initializing materials...\n");
    materials_init();
    material_cache_init();  // Build fast lookup cache
    interactions_init();
    printf("Registered %d materials\n", material_count());
    
    // Initialize world
    printf("Initializing world (%dx%d)...\n", WORLD_WIDTH, WORLD_HEIGHT);
    World world;
    world_init(&world);
    
    // Initialize renderer
    printf("Initializing renderer...\n");
    Renderer renderer;
    if (!renderer_init(&renderer)) {
        fprintf(stderr, "Failed to initialize renderer!\n");
        return 1;
    }
    
    // Initialize input
    Input input;
    input_init(&input, renderer.window);
    
    // Initialize brush
    Brush brush;
    brush_init(&brush);
    
    printf("Starting simulation...\n\n");
    
    // Timing
    f64 last_time = glfwGetTime();
    f64 accumulator = 0;
    f64 fps_timer = 0;
    i32 frame_count = 0;
    
    // Main loop
    while (!renderer_should_close(&renderer)) {
        f64 current_time = glfwGetTime();
        f64 delta_time = current_time - last_time;
        last_time = current_time;
        
        // FPS counter
        fps_timer += delta_time;
        frame_count++;
        if (fps_timer >= 1.0) {
            printf("\rFPS: %d | Material: %s | Brush: %d | %s     ", 
                   frame_count, 
                   brush_get_material_name(&brush),
                   brush.size,
                   world.paused ? "PAUSED" : "RUNNING");
            fflush(stdout);
            fps_timer = 0;
            frame_count = 0;
        }
        
        // Input
        renderer_poll_events();
        input_update(&input, &renderer);
        input_process_frame(&input, &world, &brush, &renderer);
        
        // Fixed timestep simulation
        accumulator += delta_time;
        i32 steps = 0;
        while (accumulator >= FIXED_TIMESTEP && steps < MAX_FRAME_SKIP) {
            input_apply_brush(&input, &world, &brush);
            world_update(&world);
            accumulator -= FIXED_TIMESTEP;
            steps++;
        }

        if (steps == 0) {
            input_apply_brush(&input, &world, &brush);
        }
        
        // Render
        renderer_update_texture(&renderer, &world, &brush, &input);
        renderer_draw(&renderer);
        renderer_present(&renderer);
    }
    
    printf("\n\nShutting down...\n");
    
    // Cleanup
    renderer_shutdown(&renderer);
    world_destroy(&world);
    interactions_shutdown();
    materials_shutdown();
    
    printf("Goodbye!\n");
    return 0;
}
