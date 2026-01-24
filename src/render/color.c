#include "render/color.h"
#include "world/pixel.h"
#include <math.h>
#include "materials/material_registry.h"
#include "physics/temperature.h"
#include "core/math_utils.h"
#include "core/config.h"

Color color_calculate_pixel(const Pixel* pixel, bool show_temp, bool show_pressure) {
    if (!pixel || pixel->material_id == MAT_AIR) {
        return (Color){46, 58, 70, 255}; // Soft background
    }
    
    MaterialDef* mat = material_get(pixel->material_id);
    if (!mat) return (Color){255, 0, 255, 255}; // Error: magenta
    
    // Base color with variation
    Color color = color_add_variation(mat->base_color, mat->color_variation, pixel->color_seed);

    if (pixel->material_id == MAT_FIRE && mat->lifetime > 0) {
        f32 life = (f32)pixel->lifetime / (f32)mat->lifetime;
        life = clampf(life, 0.0f, 1.0f);

        u8 flicker_seed = (u8)(pixel->color_seed + pixel->lifetime * 17u);
        f32 flicker = (f32)(flicker_seed % 20) / 19.0f;
        f32 glow = 0.2f + 0.25f * flicker;

        Color start = (Color){255, 240, 160, 255};
        Color mid = (Color){255, 140, 40, 255};
        Color end = (Color){120, 20, 0, 0};

        if (life > 0.5f) {
            f32 t = (life - 0.5f) / 0.5f;
            color = color_blend(mid, start, t);
        } else {
            f32 t = life / 0.5f;
            color = color_blend(end, mid, t);
        }

        color = color_lighten(color, glow);
        color.a = (u8)((255.0f * clampf(life * 1.2f, 0.0f, 1.0f)) * (0.7f + 0.3f * flicker));
    }
    
    // Temperature overlay
    if (show_temp) {
        Color temp_tint = temperature_get_color_tint(pixel->temperature);
        if (temp_tint.a > 0) {
            f32 blend = (f32)temp_tint.a / 255.0f;
            temp_tint.a = 255;
            color = color_blend(color, temp_tint, blend);
        }
    }
    
    // Pressure overlay
    if (show_pressure) {
        Color pressure_color = color_from_pressure(pixel->pressure);
        color = color_blend(color, pressure_color, 0.3f);
    }
    
    // Temperature-based brightness for hot materials
    if (pixel->temperature > AMBIENT_TEMPERATURE + 200) {
        f32 glow = (pixel->temperature - AMBIENT_TEMPERATURE - 200) / 1000.0f;
        glow = clampf(glow, 0, 1);
        color = color_lighten(color, glow * 0.5f);
    }
    
    return color;
}

Color color_from_temperature(f32 temperature) {
    if (temperature < 273) {
        // Cold: blue
        f32 t = (273 - temperature) / 273;
        t = clampf(t, 0, 1);
        return (Color){0, (u8)(100 * (1-t)), (u8)(200 + 55 * t), 200};
    } else if (temperature > 400) {
        // Hot: red to white
        f32 t = (temperature - 400) / 1000;
        t = clampf(t, 0, 1);
        return (Color){255, (u8)(50 + 205 * t), (u8)(255 * t), 200};
    }
    return (Color){0, 0, 0, 0};
}

Color color_from_pressure(f32 pressure) {
    f32 over_ambient = pressure - AMBIENT_PRESSURE;

    f32 normalized;
    if (over_ambient <= 0) {
        // Low pressure: linear negative scale
        normalized = over_ambient / 50000.0f;
        normalized = fmaxf(normalized, -0.5f);
    } else {
        // High pressure: logarithmic scale
        // log10(1000)=3, log10(10000)=4, log10(100000)=5, log10(1000000)=6
        // (log - 3) / 3 → 0.0 to 1.0 range
        f32 log_p = log10f(over_ambient + 1.0f);
        normalized = (log_p - 3.0f) / 3.0f;
        normalized = clampf(normalized, 0.0f, 1.0f);
    }

    // Color bands
    if (normalized < 0) {
        // Low pressure (below ambient): blue
        f32 t = -normalized * 2.0f;
        return (Color){0, (u8)(50 * t), (u8)(200 * t), (u8)(180 * t)};
    } else if (normalized < 0.3f) {
        // Low stack pressure: green tint
        f32 t = normalized / 0.3f;
        return (Color){0, (u8)(150 * t), (u8)(50 * t), (u8)(120 * t)};
    } else if (normalized < 0.6f) {
        // Medium pressure: yellow
        f32 t = (normalized - 0.3f) / 0.3f;
        return (Color){(u8)(255 * t), (u8)(200 - 50 * t), 0, (u8)(150 + 50 * t)};
    } else {
        // High pressure: orange to red
        f32 t = (normalized - 0.6f) / 0.4f;
        return (Color){255, (u8)(150 * (1 - t)), 0, (u8)(200 + 55 * t)};
    }
}

Color color_blend(Color a, Color b, f32 factor) {
    factor = clampf(factor, 0, 1);
    return (Color){
        .r = (u8)(a.r + (b.r - a.r) * factor),
        .g = (u8)(a.g + (b.g - a.g) * factor),
        .b = (u8)(a.b + (b.b - a.b) * factor),
        .a = (u8)(a.a + (b.a - a.a) * factor)
    };
}

Color color_add_variation(Color base, u8 variation_amount, u8 seed) {
    if (variation_amount == 0) return base;
    
    i32 offset = ((i32)seed - 128) * variation_amount / 255;
    return (Color){
        .r = (u8)clampi(base.r + offset, 0, 255),
        .g = (u8)clampi(base.g + offset, 0, 255),
        .b = (u8)clampi(base.b + offset, 0, 255),
        .a = base.a
    };
}

Color color_darken(Color c, f32 amount) {
    amount = clampf(amount, 0, 1);
    return (Color){
        .r = (u8)(c.r * (1 - amount)),
        .g = (u8)(c.g * (1 - amount)),
        .b = (u8)(c.b * (1 - amount)),
        .a = c.a
    };
}

Color color_lighten(Color c, f32 amount) {
    amount = clampf(amount, 0, 1);
    return (Color){
        .r = (u8)(c.r + (255 - c.r) * amount),
        .g = (u8)(c.g + (255 - c.g) * amount),
        .b = (u8)(c.b + (255 - c.b) * amount),
        .a = c.a
    };
}
