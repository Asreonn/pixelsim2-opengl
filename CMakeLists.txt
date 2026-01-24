cmake_minimum_required(VERSION 3.16)
project(PixelSim2 VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -ffast-math")

# Find threading library
find_package(Threads REQUIRED)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# GLAD - we'll include it directly
add_library(glad STATIC
    external/glad/src/glad.c
)
target_include_directories(glad PUBLIC external/glad/include)

# Main executable
set(SOURCES
    src/main.c
    src/core/config.c
    src/core/math_utils.c
    src/core/thread_pool.c
    src/physics/physics.c
    src/physics/gravity.c
    src/physics/temperature.c
    src/physics/movement.c
    src/physics/pressure.c
    src/physics/phase_transition.c
    src/world/pixel.c
    src/world/chunk.c
    src/world/world.c
    src/world/grid_ops.c
    src/materials/material_registry.c
    src/materials/material_cache.c
    src/materials/interactions.c
    src/materials/definitions/air.c
    src/materials/definitions/sand.c
    src/materials/definitions/water.c
    src/materials/definitions/stone.c
    src/materials/definitions/fire.c
    src/materials/definitions/steam.c
    src/materials/definitions/ice.c
    src/materials/definitions/glass.c
    src/materials/definitions/mud.c
    src/materials/definitions/molten_sand.c
    src/materials/definitions/metal.c
    src/render/renderer.c
    src/render/shader.c
    src/render/color.c
    src/input/input.c
    src/input/brush.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/glad/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    glad
    m
    Threads::Threads
)

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/${PROJECT_NAME}/assets)
