#include "materials/material_registry.h"
#include "core/config.h"
#include <string.h>

static MaterialDef* materials[MAX_MATERIALS] = {0};
static u16 material_count_val = 0;

void materials_init(void) {
    memset(materials, 0, sizeof(materials));
    material_count_val = 0;
    
    // Register all built-in materials
    material_register(&MAT_DEF_AIR);
    material_register(&MAT_DEF_SAND);
    material_register(&MAT_DEF_WATER);
    material_register(&MAT_DEF_STONE);
    material_register(&MAT_DEF_FIRE);
    material_register(&MAT_DEF_STEAM);
    material_register(&MAT_DEF_ICE);
    material_register(&MAT_DEF_GLASS);
    material_register(&MAT_DEF_MUD);
}

void materials_shutdown(void) {
    memset(materials, 0, sizeof(materials));
    material_count_val = 0;
}

bool material_register(const MaterialDef* def) {
    if (!def || def->id >= MAX_MATERIALS) return false;
    
    materials[def->id] = (MaterialDef*)def;
    if (def->id >= material_count_val) {
        material_count_val = def->id + 1;
    }
    return true;
}

MaterialDef* material_get(u16 id) {
    if (id >= MAX_MATERIALS) return NULL;
    return materials[id];
}

const char* material_get_name(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->name : "Unknown";
}

MaterialState material_get_state(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->base_state : STATE_SOLID;
}

f32 material_get_density(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->density : 0;
}

f32 material_get_viscosity(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->viscosity : 0;
}

f32 material_get_thermal_conductivity(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->thermal_conductivity : 0;
}

bool material_has_flag(u16 id, u32 flag) {
    MaterialDef* mat = material_get(id);
    return mat ? (mat->flags & flag) != 0 : false;
}

bool material_is_flammable(u16 id) {
    return material_has_flag(id, MAT_FLAG_FLAMMABLE);
}

bool material_emits_heat(u16 id) {
    return material_has_flag(id, MAT_FLAG_EMITS_HEAT);
}

u16 material_get_melts_into(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->melts_into : MAT_AIR;
}

u16 material_get_boils_into(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->boils_into : MAT_AIR;
}

u16 material_get_freezes_into(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->freezes_into : MAT_AIR;
}

u16 material_get_burns_into(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->burns_into : MAT_AIR;
}

Color material_get_base_color(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->base_color : (Color){0, 0, 0, 255};
}

u8 material_get_color_variation(u16 id) {
    MaterialDef* mat = material_get(id);
    return mat ? mat->color_variation : 0;
}

u16 material_count(void) {
    return material_count_val;
}
