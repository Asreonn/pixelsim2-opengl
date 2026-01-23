#include "render/renderer.h"
#include "render/shader.h"
#include "render/color.h"
#include "world/world.h"
#include "world/pixel.h"
#include "materials/material_registry.h"
#include "input/brush.h"
#include "core/config.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char* VERTEX_SHADER = 
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform vec2 offset;\n"
    "uniform float zoom;\n"
    "void main() {\n"
    "    gl_Position = vec4((aPos + offset) * zoom, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

static const char* FRAGMENT_SHADER =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D worldTexture;\n"
    "void main() {\n"
    "    FragColor = texture(worldTexture, TexCoord);\n"
    "}\n";

static const u8 FONT_5X7[37][7] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}
};

static const u8* renderer_get_glyph(char c) {
    if (c == ' ') return FONT_5X7[0];
    if (c >= '0' && c <= '9') return FONT_5X7[1 + (c - '0')];
    if (c >= 'A' && c <= 'Z') return FONT_5X7[11 + (c - 'A')];
    return FONT_5X7[0];
}

static void renderer_draw_char(Renderer* renderer, i32 x, i32 y, char c, Color color) {
    const u8* glyph = renderer_get_glyph(c);
    for (i32 row = 0; row < 7; row++) {
        u8 bits = glyph[row];
        for (i32 col = 0; col < 5; col++) {
            if ((bits & (1u << (4 - col))) == 0) continue;
            i32 px = x + col;
            i32 py = y + row;
            if (px < 0 || px >= WORLD_WIDTH || py < 0 || py >= WORLD_HEIGHT) continue;
            renderer->pixel_buffer[py][px][0] = color.r;
            renderer->pixel_buffer[py][px][1] = color.g;
            renderer->pixel_buffer[py][px][2] = color.b;
            renderer->pixel_buffer[py][px][3] = color.a;
        }
    }
}

static void renderer_draw_text(Renderer* renderer, i32 x, i32 y, const char* text, Color color) {
    i32 cursor_x = x;
    for (size_t i = 0; text[i] != '\0'; i++) {
        char c = (char)toupper((unsigned char)text[i]);
        renderer_draw_char(renderer, cursor_x, y, c, color);
        cursor_x += 6;
    }
}

static void renderer_draw_text_shadow(Renderer* renderer, i32 x, i32 y, const char* text, Color color) {
    Color shadow = (Color){0, 0, 0, 200};
    renderer_draw_text(renderer, x + 1, y + 1, text, shadow);
    renderer_draw_text(renderer, x, y, text, color);
}

static const char* renderer_state_name(MaterialState state) {
    switch (state) {
        case STATE_SOLID:
            return "SOLID";
        case STATE_POWDER:
            return "POWDER";
        case STATE_LIQUID:
            return "LIQUID";
        case STATE_GAS:
            return "GAS";
        case STATE_PLASMA:
            return "PLASMA";
        default:
            return "UNKNOWN";
    }
}

static i32 renderer_temp_to_celsius(f32 temp_k) {
    return (i32)(temp_k - 273.15f + (temp_k >= 273.15f ? 0.5f : -0.5f));
}

static void renderer_draw_hud(Renderer* renderer, World* world, Brush* brush) {
    if (!world || !brush) return;

    Color text_color = (Color){230, 230, 230, 230};
    char line[64];
    i32 y = 4;

    i32 ambient_c = renderer_temp_to_celsius(world->ambient_temperature);
    snprintf(line, sizeof(line), "AMBIENT %dC", ambient_c);
    renderer_draw_text_shadow(renderer, 4, y, line, text_color);
    y += 9;

    const char* mat_name = material_get_name(brush->material_id);
    snprintf(line, sizeof(line), "BRUSH %s", mat_name ? mat_name : "UNKNOWN");
    renderer_draw_text_shadow(renderer, 4, y, line, text_color);
    y += 9;

    MaterialState state = material_get_state(brush->material_id);
    snprintf(line, sizeof(line), "STATE %s", renderer_state_name(state));
    renderer_draw_text_shadow(renderer, 4, y, line, text_color);
    y += 9;

    i32 brush_c = renderer_temp_to_celsius(brush->temperature);
    snprintf(line, sizeof(line), "TEMP %dC", brush_c);
    renderer_draw_text_shadow(renderer, 4, y, line, text_color);
    y += 9;

    snprintf(line, sizeof(line), "SIZE %d", brush->size);
    renderer_draw_text_shadow(renderer, 4, y, line, text_color);
}

bool renderer_init(Renderer* renderer) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    renderer->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!renderer->window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(renderer->window);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return false;
    }
    
    // Setup viewport
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Create shader
    renderer->shader_program = shader_load_from_source(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!renderer->shader_program) return false;
    
    // Create quad vertices
    float vertices[] = {
        // pos        // tex
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };
    
    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    glGenBuffers(1, &renderer->ebo);
    
    glBindVertexArray(renderer->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Create texture
    glGenTextures(1, &renderer->texture);
    glBindTexture(GL_TEXTURE_2D, renderer->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    memset(renderer->pixel_buffer, 0, sizeof(renderer->pixel_buffer));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WORLD_WIDTH, WORLD_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, renderer->pixel_buffer);
    
    renderer->show_temperature = false;
    renderer->show_pressure = false;
    renderer->zoom = 1.0f;
    renderer->offset_x = 0;
    renderer->offset_y = 0;
    
    return true;
}

void renderer_shutdown(Renderer* renderer) {
    glDeleteVertexArrays(1, &renderer->vao);
    glDeleteBuffers(1, &renderer->vbo);
    glDeleteBuffers(1, &renderer->ebo);
    glDeleteTextures(1, &renderer->texture);
    shader_delete(renderer->shader_program);
    glfwDestroyWindow(renderer->window);
    glfwTerminate();
}

void renderer_update_texture(Renderer* renderer, World* world, Brush* brush) {
    for (i32 y = 0; y < WORLD_HEIGHT; y++) {
        for (i32 x = 0; x < WORLD_WIDTH; x++) {
            Pixel* pixel = world_get_pixel(world, x, y);
            Color color = pixel ? color_calculate_pixel(pixel, renderer->show_temperature, renderer->show_pressure) 
                               : (Color){30, 30, 40, 255};
            
            renderer->pixel_buffer[y][x][0] = color.r;
            renderer->pixel_buffer[y][x][1] = color.g;
            renderer->pixel_buffer[y][x][2] = color.b;
            renderer->pixel_buffer[y][x][3] = color.a;
        }
    }

    renderer_draw_hud(renderer, world, brush);
    
    glBindTexture(GL_TEXTURE_2D, renderer->texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WORLD_WIDTH, WORLD_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, renderer->pixel_buffer);
}

void renderer_update_chunk_texture(Renderer* renderer, World* world, i32 chunk_x, i32 chunk_y) {
    (void)renderer; (void)world; (void)chunk_x; (void)chunk_y;
    // For optimization later
}

void renderer_draw(Renderer* renderer) {
    glClearColor(0.18f, 0.23f, 0.28f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    shader_use(renderer->shader_program);
    shader_set_vec2(renderer->shader_program, "offset", renderer->offset_x, renderer->offset_y);
    shader_set_float(renderer->shader_program, "zoom", renderer->zoom);
    
    glBindTexture(GL_TEXTURE_2D, renderer->texture);
    glBindVertexArray(renderer->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void renderer_present(Renderer* renderer) {
    glfwSwapBuffers(renderer->window);
}

void renderer_set_zoom(Renderer* renderer, f32 zoom) {
    renderer->zoom = zoom;
}

void renderer_set_offset(Renderer* renderer, f32 x, f32 y) {
    renderer->offset_x = x;
    renderer->offset_y = y;
}

void renderer_toggle_temperature_view(Renderer* renderer) {
    renderer->show_temperature = !renderer->show_temperature;
}

void renderer_toggle_pressure_view(Renderer* renderer) {
    renderer->show_pressure = !renderer->show_pressure;
}

void renderer_screen_to_world(Renderer* renderer, f32 screen_x, f32 screen_y, i32* world_x, i32* world_y) {
    *world_x = (i32)(screen_x);
    *world_y = (i32)(screen_y);
}

bool renderer_should_close(Renderer* renderer) {
    return glfwWindowShouldClose(renderer->window);
}

void renderer_poll_events(void) {
    glfwPollEvents();
}
