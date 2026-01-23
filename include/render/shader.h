#ifndef PIXELSIM_SHADER_H
#define PIXELSIM_SHADER_H

#include "core/types.h"

// Load and compile shader program from files
u32 shader_load(const char* vertex_path, const char* fragment_path);

// Load and compile shader program from source strings
u32 shader_load_from_source(const char* vertex_src, const char* fragment_src);

// Delete shader program
void shader_delete(u32 program);

// Use shader program
void shader_use(u32 program);

// Set uniforms
void shader_set_int(u32 program, const char* name, i32 value);
void shader_set_float(u32 program, const char* name, f32 value);
void shader_set_vec2(u32 program, const char* name, f32 x, f32 y);
void shader_set_vec3(u32 program, const char* name, f32 x, f32 y, f32 z);
void shader_set_vec4(u32 program, const char* name, f32 x, f32 y, f32 z, f32 w);

#endif
