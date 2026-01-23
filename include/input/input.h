#ifndef PIXELSIM_INPUT_H
#define PIXELSIM_INPUT_H

#include "core/types.h"

struct GLFWwindow;
struct Renderer;
struct World;
struct Brush;

// Input state
typedef struct Input {
    // Mouse state
    f32 mouse_x, mouse_y;
    f32 mouse_dx, mouse_dy;
    bool mouse_left;
    bool mouse_right;
    bool mouse_middle;
    f32 scroll_delta;
    
    // Key states
    bool keys[512];
    bool keys_pressed[512];  // Just pressed this frame
    
    // World coordinates
    i32 world_x, world_y;
    bool in_bounds;
    i32 prev_world_x, prev_world_y;
    bool prev_in_bounds;
} Input;

// Initialize input system
void input_init(Input* input, struct GLFWwindow* window);

// Update input state (call once per frame)
void input_update(Input* input, struct Renderer* renderer);

// Process input that should happen once per frame
void input_process_frame(Input* input, struct World* world, struct Brush* brush, struct Renderer* renderer);

// Apply brush effects (call per simulation step)
void input_apply_brush(Input* input, struct World* world, struct Brush* brush);

// Key queries
bool input_key_down(Input* input, i32 key);
bool input_key_pressed(Input* input, i32 key);

// Mouse queries
bool input_mouse_in_world(Input* input);
void input_get_world_pos(Input* input, i32* x, i32* y);

// GLFW callbacks (set up internally)
void input_setup_callbacks(struct GLFWwindow* window);

#endif
