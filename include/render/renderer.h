#ifndef PIXELSIM_RENDERER_H
#define PIXELSIM_RENDERER_H

#include "core/types.h"
#include "core/config.h"

struct World;
struct Brush;
struct GLFWwindow;
struct Input;

// Renderer state
typedef struct Renderer {
    struct GLFWwindow* window;
    u32 shader_program;
    u32 vao, vbo, ebo;
    u32 texture;
    
    // PBO double buffering for async texture upload
    u32 pbo[2];
    i32 pbo_index;
    void* pbo_sync;  // GLsync fence
    
    u8 pixel_buffer[WORLD_HEIGHT][WORLD_WIDTH][4];  // RGBA buffer
    bool show_temperature;
    bool show_pressure;
    f32 zoom;
    f32 offset_x, offset_y;
    
    // Dirty chunk tracking
    bool chunk_dirty[CHUNKS_Y][CHUNKS_X];
    bool use_pbo;  // Whether PBO is available/enabled

    // UI Panel
    u32 ui_texture;
    u8 ui_buffer[UI_PANEL_HEIGHT][WINDOW_WIDTH][4];
    bool ui_needs_redraw;
} Renderer;

// Lifecycle
bool renderer_init(Renderer* renderer);
void renderer_shutdown(Renderer* renderer);

// Update texture from world
void renderer_update_texture(Renderer* renderer, struct World* world, struct Brush* brush, struct Input* input);
void renderer_update_chunk_texture(Renderer* renderer, struct World* world, i32 chunk_x, i32 chunk_y);

// Render
void renderer_draw(Renderer* renderer);
void renderer_present(Renderer* renderer);

// View controls
void renderer_set_zoom(Renderer* renderer, f32 zoom);
void renderer_set_offset(Renderer* renderer, f32 x, f32 y);
void renderer_toggle_temperature_view(Renderer* renderer);
void renderer_toggle_pressure_view(Renderer* renderer);

// Coordinate conversion
void renderer_screen_to_world(Renderer* renderer, f32 screen_x, f32 screen_y, i32* world_x, i32* world_y);

// Window
bool renderer_should_close(Renderer* renderer);
void renderer_poll_events(void);

// UI Panel
bool renderer_check_ui_click(i32 screen_x, i32 screen_y, struct Brush* brush);

#endif
