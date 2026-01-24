#ifndef PIXELSIM_CONFIG_H
#define PIXELSIM_CONFIG_H

// World dimensions
#define WORLD_WIDTH 512
#define WORLD_HEIGHT 512
#define CHUNK_SIZE 64
#define CHUNKS_X (WORLD_WIDTH / CHUNK_SIZE)
#define CHUNKS_Y (WORLD_HEIGHT / CHUNK_SIZE)

// Window
#define UI_PANEL_HEIGHT 68
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT (512 + UI_PANEL_HEIGHT)
#define WORLD_VIEWPORT_HEIGHT 512
#define WINDOW_TITLE "PixelSim2 - Physics Simulator"

// Simulation
#define TICKS_PER_SECOND 60
#define FIXED_TIMESTEP (1.0f / TICKS_PER_SECOND)
#define MAX_FRAME_SKIP 5

// Physics constants
#define GRAVITY 9.81f
#define THERMAL_LIFT_CONSTANT 0.05f
#define COOLING_RATE 0.35f
#define HEAT_TRANSFER_SCALE 20000.0f
#define MAX_SPREAD 5
#define MAX_VELOCITY 10.0f

// Temperature (Kelvin)
#define AMBIENT_TEMPERATURE 293.15f
#define FIRE_TEMPERATURE 1200.0f
#define FIRE_HEAT_OUTPUT 50.0f
#define MIN_TEMPERATURE 0.0f
#define MAX_TEMPERATURE 5000.0f

// Pressure
#define AMBIENT_PRESSURE 101325.0f
#define EXPLOSION_THRESHOLD 500000.0f
#define PRESSURE_TRANSFER_RATE 0.1f

// Materials
#define MAX_MATERIALS 256
#define MAX_INTERACTIONS 512

// Chunk optimization
#define CHUNK_SLEEP_THRESHOLD 60

// Brush defaults
#define DEFAULT_BRUSH_SIZE 3
#define MIN_BRUSH_SIZE 1
#define MAX_BRUSH_SIZE 50

#endif
