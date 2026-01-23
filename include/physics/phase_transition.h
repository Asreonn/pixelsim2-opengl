#ifndef PIXELSIM_PHASE_TRANSITION_H
#define PIXELSIM_PHASE_TRANSITION_H

#include "core/types.h"

struct World;
struct Pixel;

// Check and apply phase transitions for a pixel
// Returns true if a transition occurred
bool phase_check_transitions(struct World* world, i32 x, i32 y);

// Individual transition checks
bool phase_check_melting(struct World* world, i32 x, i32 y);
bool phase_check_freezing(struct World* world, i32 x, i32 y);
bool phase_check_boiling(struct World* world, i32 x, i32 y);
bool phase_check_condensation(struct World* world, i32 x, i32 y);
bool phase_check_combustion(struct World* world, i32 x, i32 y);

// Transform pixel to another material (preserves some properties)
void phase_transform(struct World* world, i32 x, i32 y, u16 new_material);

// Fire-specific updates
void phase_update_fire(struct World* world, i32 x, i32 y);

// Try to ignite neighboring flammable materials
void phase_spread_fire(struct World* world, i32 x, i32 y);

#endif
