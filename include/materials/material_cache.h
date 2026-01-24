#ifndef PIXELSIM_MATERIAL_CACHE_H
#define PIXELSIM_MATERIAL_CACHE_H

#include "core/types.h"
#include "core/config.h"

// Pre-computed material properties for fast lookup
// Eliminates function call overhead in hot paths
typedef struct MaterialCache {
    MaterialState states[MAX_MATERIALS];
    f32 densities[MAX_MATERIALS];
    f32 viscosities[MAX_MATERIALS];
    f32 frictions[MAX_MATERIALS];
    f32 dispersions[MAX_MATERIALS];
    f32 thermal_conductivities[MAX_MATERIALS];
    f32 specific_heats[MAX_MATERIALS];
    f32 melting_points[MAX_MATERIALS];
    f32 boiling_points[MAX_MATERIALS];
    f32 freezing_points[MAX_MATERIALS];
    f32 ignition_points[MAX_MATERIALS];
    u16 melts_into[MAX_MATERIALS];
    u16 boils_into[MAX_MATERIALS];
    u16 freezes_into[MAX_MATERIALS];
    u16 burns_into[MAX_MATERIALS];
    u32 flags[MAX_MATERIALS];
    u8 lifetimes[MAX_MATERIALS];
    Color base_colors[MAX_MATERIALS];
    u8 color_variations[MAX_MATERIALS];
} MaterialCache;

// Global material cache (defined in material_cache.c)
extern MaterialCache g_mat_cache;

// Initialize cache from registered materials
void material_cache_init(void);

// Fast inline lookups - no function call overhead
#define MAT_CACHE_STATE(id)              (g_mat_cache.states[id])
#define MAT_CACHE_DENSITY(id)            (g_mat_cache.densities[id])
#define MAT_CACHE_VISCOSITY(id)          (g_mat_cache.viscosities[id])
#define MAT_CACHE_FRICTION(id)           (g_mat_cache.frictions[id])
#define MAT_CACHE_DISPERSION(id)         (g_mat_cache.dispersions[id])
#define MAT_CACHE_THERMAL_K(id)          (g_mat_cache.thermal_conductivities[id])
#define MAT_CACHE_SPECIFIC_HEAT(id)      (g_mat_cache.specific_heats[id])
#define MAT_CACHE_MELTING_POINT(id)      (g_mat_cache.melting_points[id])
#define MAT_CACHE_BOILING_POINT(id)      (g_mat_cache.boiling_points[id])
#define MAT_CACHE_FREEZING_POINT(id)     (g_mat_cache.freezing_points[id])
#define MAT_CACHE_IGNITION_POINT(id)     (g_mat_cache.ignition_points[id])
#define MAT_CACHE_MELTS_INTO(id)         (g_mat_cache.melts_into[id])
#define MAT_CACHE_BOILS_INTO(id)         (g_mat_cache.boils_into[id])
#define MAT_CACHE_FREEZES_INTO(id)       (g_mat_cache.freezes_into[id])
#define MAT_CACHE_BURNS_INTO(id)         (g_mat_cache.burns_into[id])
#define MAT_CACHE_FLAGS(id)              (g_mat_cache.flags[id])
#define MAT_CACHE_LIFETIME(id)           (g_mat_cache.lifetimes[id])
#define MAT_CACHE_BASE_COLOR(id)         (g_mat_cache.base_colors[id])
#define MAT_CACHE_COLOR_VAR(id)          (g_mat_cache.color_variations[id])

// Flag checks
#define MAT_CACHE_IS_FLAMMABLE(id)       ((g_mat_cache.flags[id] & MAT_FLAG_FLAMMABLE) != 0)
#define MAT_CACHE_EMITS_HEAT(id)         ((g_mat_cache.flags[id] & MAT_FLAG_EMITS_HEAT) != 0)
#define MAT_CACHE_IS_GAS(id)             (g_mat_cache.states[id] == STATE_GAS || g_mat_cache.states[id] == STATE_PLASMA)
#define MAT_CACHE_IS_LIQUID(id)          (g_mat_cache.states[id] == STATE_LIQUID)
#define MAT_CACHE_IS_SOLID(id)           (g_mat_cache.states[id] == STATE_SOLID)
#define MAT_CACHE_IS_POWDER(id)          (g_mat_cache.states[id] == STATE_POWDER)

#endif
