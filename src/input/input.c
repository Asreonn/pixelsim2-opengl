#include "input/input.h"
#include "input/brush.h"
#include "materials/material_ids.h"
#include "render/renderer.h"
#include "world/world.h"
#include "core/math_utils.h"
#include <GLFW/glfw3.h>
#include <string.h>

static Input* current_input = NULL;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)scancode; (void)mods;
    if (!current_input || key < 0 || key >= 512) return;
    
    if (action == GLFW_PRESS) {
        current_input->keys[key] = true;
        current_input->keys_pressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        current_input->keys[key] = false;
    }
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window; (void)xoffset;
    if (current_input) {
        current_input->scroll_delta = (f32)yoffset;
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (!current_input) return;
    
    bool pressed = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_LEFT) current_input->mouse_left = pressed;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) current_input->mouse_right = pressed;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) current_input->mouse_middle = pressed;
}

static void input_apply_brush_line(Brush* brush, World* world, i32 x0, i32 y0, i32 x1, i32 y1, bool erase) {
    i32 dx = absi(x1 - x0);
    i32 sx = x0 < x1 ? 1 : -1;
    i32 dy = -absi(y1 - y0);
    i32 sy = y0 < y1 ? 1 : -1;
    i32 err = dx + dy;

    for (;;) {
        if (world_in_bounds(x0, y0)) {
            if (erase) {
                brush_erase(brush, world, x0, y0);
            } else {
                brush_paint(brush, world, x0, y0);
            }
        }

        if (x0 == x1 && y0 == y1) break;

        i32 e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void input_init(Input* input, GLFWwindow* window) {
    memset(input, 0, sizeof(Input));
    current_input = input;
    input_setup_callbacks(window);
}

void input_setup_callbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
}

void input_update(Input* input, Renderer* renderer) {
    input->prev_world_x = input->world_x;
    input->prev_world_y = input->world_y;
    input->prev_in_bounds = input->in_bounds;

    // Get mouse position
    double mx, my;
    glfwGetCursorPos(renderer->window, &mx, &my);
    
    input->mouse_dx = (f32)mx - input->mouse_x;
    input->mouse_dy = (f32)my - input->mouse_y;
    input->mouse_x = (f32)mx;
    input->mouse_y = (f32)my;
    
    // Convert to world coordinates
    input->world_x = (i32)mx;
    input->world_y = (i32)my;
    input->in_bounds = (input->world_x >= 0 && input->world_x < WORLD_WIDTH &&
                        input->world_y >= 0 && input->world_y < WORLD_HEIGHT);
}

void input_apply_brush(Input* input, World* world, Brush* brush) {
    if (input->mouse_left) {
        if (input->in_bounds && input->prev_in_bounds) {
            input_apply_brush_line(brush, world, input->prev_world_x, input->prev_world_y,
                                   input->world_x, input->world_y, false);
        } else if (input->in_bounds) {
            brush_paint(brush, world, input->world_x, input->world_y);
        }
    }
    if (input->mouse_right) {
        if (input->in_bounds && input->prev_in_bounds) {
            input_apply_brush_line(brush, world, input->prev_world_x, input->prev_world_y,
                                   input->world_x, input->world_y, true);
        } else if (input->in_bounds) {
            brush_erase(brush, world, input->world_x, input->world_y);
        }
    }
}

void input_process_frame(Input* input, World* world, Brush* brush, Renderer* renderer) {
    if (input->scroll_delta != 0) {
        if (input->scroll_delta > 0) brush_increase_size(brush);
        else brush_decrease_size(brush);
    }

    static const u16 material_key_map[9] = {
        MAT_SAND, MAT_WATER, MAT_STONE, MAT_FIRE, MAT_STEAM,
        MAT_ICE, MAT_GLASS, MAT_MUD, MAT_AIR
    };
    static const i32 material_keys[9] = {
        GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5,
        GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9
    };
    static const i32 material_keypad_keys[9] = {
        GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4, GLFW_KEY_KP_5,
        GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9
    };
    for (int i = 0; i < 9; i++) {
        if (input->keys_pressed[material_keys[i]] || input->keys_pressed[material_keypad_keys[i]]) {
            brush_set_material(brush, material_key_map[i]);
        }
    }

    if (input->keys_pressed[GLFW_KEY_SPACE]) {
        world_toggle_pause(world);
    }

    if (input->keys_pressed[GLFW_KEY_R]) {
        world_clear(world);
    }

    if (input->keys_pressed[GLFW_KEY_T]) {
        renderer_toggle_temperature_view(renderer);
    }

    if (input->keys_pressed[GLFW_KEY_P]) {
        renderer_toggle_pressure_view(renderer);
    }

    memset(input->keys_pressed, 0, sizeof(input->keys_pressed));
    input->scroll_delta = 0;
}

bool input_key_down(Input* input, i32 key) {
    if (key < 0 || key >= 512) return false;
    return input->keys[key];
}

bool input_key_pressed(Input* input, i32 key) {
    if (key < 0 || key >= 512) return false;
    return input->keys_pressed[key];
}

bool input_mouse_in_world(Input* input) {
    return input->in_bounds;
}

void input_get_world_pos(Input* input, i32* x, i32* y) {
    *x = input->world_x;
    *y = input->world_y;
}
