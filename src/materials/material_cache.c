#include "materials/material_cache.h"
#include "materials/material_registry.h"
#include <string.h>

// Global material cache instance
MaterialCache g_mat_cache;

void material_cache_init(void) {
    memset(&g_mat_cache, 0, sizeof(g_mat_cache));
    
    u16 count = material_count();
    
    for (u16 id = 0; id < count; id++) {
        MaterialDef* mat = material_get(id);
        if (!mat) continue;
        
        g_mat_cache.states[id] = mat->base_state;
        g_mat_cache.densities[id] = mat->density;
        g_mat_cache.viscosities[id] = mat->viscosity;
        g_mat_cache.frictions[id] = mat->friction;
        g_mat_cache.dispersions[id] = mat->dispersion;
        g_mat_cache.thermal_conductivities[id] = mat->thermal_conductivity;
        g_mat_cache.specific_heats[id] = mat->specific_heat;
        g_mat_cache.melting_points[id] = mat->melting_point;
        g_mat_cache.boiling_points[id] = mat->boiling_point;
        g_mat_cache.freezing_points[id] = mat->freezing_point;
        g_mat_cache.ignition_points[id] = mat->ignition_point;
        g_mat_cache.melts_into[id] = mat->melts_into;
        g_mat_cache.boils_into[id] = mat->boils_into;
        g_mat_cache.freezes_into[id] = mat->freezes_into;
        g_mat_cache.burns_into[id] = mat->burns_into;
        g_mat_cache.flags[id] = mat->flags;
        g_mat_cache.lifetimes[id] = mat->lifetime;
        g_mat_cache.base_colors[id] = mat->base_color;
        g_mat_cache.color_variations[id] = mat->color_variation;
    }
}
