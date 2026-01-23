#include "render/shader.h"
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) { fclose(file); return NULL; }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

static u32 compile_shader(const char* source, GLenum type) {
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
        return 0;
    }
    return shader;
}

u32 shader_load(const char* vertex_path, const char* fragment_path) {
    char* vert_src = read_file(vertex_path);
    char* frag_src = read_file(fragment_path);
    
    if (!vert_src || !frag_src) {
        free(vert_src);
        free(frag_src);
        return 0;
    }
    
    u32 program = shader_load_from_source(vert_src, frag_src);
    free(vert_src);
    free(frag_src);
    return program;
}

u32 shader_load_from_source(const char* vertex_src, const char* fragment_src) {
    u32 vert = compile_shader(vertex_src, GL_VERTEX_SHADER);
    u32 frag = compile_shader(fragment_src, GL_FRAGMENT_SHADER);
    
    if (!vert || !frag) return 0;
    
    u32 program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        fprintf(stderr, "Shader link error: %s\n", log);
        return 0;
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

void shader_delete(u32 program) {
    glDeleteProgram(program);
}

void shader_use(u32 program) {
    glUseProgram(program);
}

void shader_set_int(u32 program, const char* name, i32 value) {
    glUniform1i(glGetUniformLocation(program, name), value);
}

void shader_set_float(u32 program, const char* name, f32 value) {
    glUniform1f(glGetUniformLocation(program, name), value);
}

void shader_set_vec2(u32 program, const char* name, f32 x, f32 y) {
    glUniform2f(glGetUniformLocation(program, name), x, y);
}

void shader_set_vec3(u32 program, const char* name, f32 x, f32 y, f32 z) {
    glUniform3f(glGetUniformLocation(program, name), x, y, z);
}

void shader_set_vec4(u32 program, const char* name, f32 x, f32 y, f32 z, f32 w) {
    glUniform4f(glGetUniformLocation(program, name), x, y, z, w);
}
