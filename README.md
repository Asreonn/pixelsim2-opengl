# PixelSim2: falling-sand physics in C + OpenGL

Second version of my pixel physics sandbox. Version 1 ([pixel-physics-sim](https://github.com/Asreonn/pixel-physics-sim)) used SDL2. This rewrite moves rendering to OpenGL (GLFW + GLAD), splits the world into chunks and adds temperature, pressure and phase changes.

> **Status:** Archived. Finished in January 2026. About 4k lines of C11.

## Features

- 512×512 cell world split into 64×64 chunks; idle chunks go to sleep so only active areas are simulated.
- Data-driven materials, one file each in `src/materials/definitions/`: sand, water, stone, fire, steam, ice, glass, mud, metal, molten sand and air.
- Physics modules: gravity and movement, temperature diffusion, pressure, and phase transitions (ice ↔ water ↔ steam, sand → molten sand → glass…), plus material interaction rules.
- Thread pool for the update step (`src/core/thread_pool.c`).
- OpenGL renderer with temperature and pressure debug views.

## Controls

| Input | Action |
|---|---|
| Left mouse | Paint the selected material |
| Right mouse | Erase |
| Mouse wheel | Brush size |
| `1`–`9` | Sand, water, stone, fire, steam, ice, glass, mud, air |
| `Space` | Pause / resume |
| `R` | Clear the world |
| `T` / `P` | Toggle temperature / pressure view |

## Build and run

Needs CMake ≥ 3.16, a C11 compiler, OpenGL and GLFW 3.3+ (GLAD is vendored in `external/glad`).

```bash
# Fedora: sudo dnf install cmake gcc glfw-devel mesa-libGL-devel
# Ubuntu: sudo apt install cmake build-essential libglfw3-dev libgl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/PixelSim2
```

Release builds use `-O3 -march=native -ffast-math`.

## Layout

```
include/ and src/
  core/       config, math utils, thread pool
  world/      world, chunks, pixels, grid ops
  materials/  registry, cache, interactions, definitions/
  physics/    gravity, movement, temperature, pressure, phase transitions
  render/     OpenGL renderer, shaders, colours
  input/      mouse/keyboard, brush
external/glad OpenGL loader
```

Note: a local `build/` directory was committed by mistake. It is not needed; build into a fresh directory as shown above.
