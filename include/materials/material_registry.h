#ifndef PIXELSIM_MATERIAL_REGISTRY_H
#define PIXELSIM_MATERIAL_REGISTRY_H

#include "core/types.h"
#include "materials/material_def.h"
#include "materials/material_ids.h"

// Initialize material registry and register all materials
void         materials_init(void);
void         materials_shutdown(void);

// Register a new material
bool         material_register(const MaterialDef* def);

// Get material definition by ID
MaterialDef* material_get(u16 id);
const char*  material_get_name(u16 id);
MaterialState material_get_state(u16 id);

// Material queries
f32          material_get_density(u16 id);
f32          material_get_viscosity(u16 id);
f32          material_get_thermal_conductivity(u16 id);
bool         material_has_flag(u16 id, u32 flag);
bool         material_is_flammable(u16 id);
bool         material_emits_heat(u16 id);

// Phase transitions
u16          material_get_melts_into(u16 id);
u16          material_get_boils_into(u16 id);
u16          material_get_freezes_into(u16 id);
u16          material_get_burns_into(u16 id);

// Color
Color        material_get_base_color(u16 id);
u8           material_get_color_variation(u16 id);

// Count
u16          material_count(void);

// Material definition declarations (defined in each material's .c file)
extern MaterialDef MAT_DEF_AIR;
extern MaterialDef MAT_DEF_SAND;
extern MaterialDef MAT_DEF_WATER;
extern MaterialDef MAT_DEF_STONE;
extern MaterialDef MAT_DEF_FIRE;
extern MaterialDef MAT_DEF_STEAM;
extern MaterialDef MAT_DEF_ICE;
extern MaterialDef MAT_DEF_GLASS;
extern MaterialDef MAT_DEF_MUD;

#endif
