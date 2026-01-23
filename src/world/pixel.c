#include "world/pixel.h"
#include "core/config.h"
#include "core/math_utils.h"
#include "materials/material_registry.h"
#include "render/color.h"

void pixel_init(Pixel* pixel, u16 material_id) {
    pixel_init_with_temp(pixel, material_id, AMBIENT_TEMPERATURE);
}

void pixel_init_with_temp(Pixel* pixel, u16 material_id, f32 temperature) {
    pixel->material_id = material_id;
    pixel->temperature = temperature;
    pixel->velocity_x = 0.0f;
    pixel->velocity_y = 0.0f;
    pixel->pressure = AMBIENT_PRESSURE;
    pixel->lifetime = material_get(material_id) ? material_get(material_id)->lifetime : 0;
    pixel->updated = 0;
    pixel->color_seed = (u8)rand_u32();
    pixel->flags = 0;
}

void pixel_reset(Pixel* pixel) {
    pixel_init(pixel, MAT_AIR);
}

void pixel_copy(Pixel* dest, const Pixel* src) {
    *dest = *src;
}

bool pixel_is_empty(const Pixel* pixel) {
    return pixel->material_id == MAT_AIR;
}

bool pixel_is_solid(const Pixel* pixel) {
    MaterialState state = material_get_state(pixel->material_id);
    return state == STATE_SOLID;
}

bool pixel_is_liquid(const Pixel* pixel) {
    MaterialState state = material_get_state(pixel->material_id);
    return state == STATE_LIQUID;
}

bool pixel_is_gas(const Pixel* pixel) {
    MaterialState state = material_get_state(pixel->material_id);
    return state == STATE_GAS || state == STATE_PLASMA;
}

bool pixel_is_powder(const Pixel* pixel) {
    MaterialState state = material_get_state(pixel->material_id);
    return state == STATE_POWDER;
}

Color pixel_get_color(const Pixel* pixel) {
    return color_calculate_pixel(pixel, false, false);
}
