#include "rsx_frag_present.h"

void rsx_frag_present_from_surface(rsx_frag_present_req* out, const rsx_frag_surface* s) {
    if (!out) return;
    out->color_offset = s ? s->color_offset : 0;
    out->pitch = s && s->color_pitch ? s->color_pitch : (s && s->width ? s->width * 4u : 720u * 4u);
    out->width = s && s->width ? s->width : 720u;
    out->height = s && s->height ? s->height : 576u;
    out->flags = out->color_offset ? 2u : 1u;
}

void rsx_frag_present_fill_pattern(uint8_t* mem, uint64_t mem_size,
                                   uint32_t offset, uint32_t w, uint32_t h, uint32_t frame_id) {
    if (!mem || !w || !h) return;
    uint64_t need = (uint64_t)offset + (uint64_t)w * (uint64_t)h * 4u;
    if (need > mem_size) return;
    uint8_t* dst = mem + offset;
    uint32_t phase = frame_id * 5u;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint32_t i = (y * w + x) * 4u;
            uint8_t r = (uint8_t)((x * 2u + phase) & 255);
            uint8_t g = (uint8_t)((y * 2u + phase * 3u) & 255);
            uint8_t b = (uint8_t)(((x + y) + phase) & 255);
            if (((x >> 4) ^ (y >> 4) ^ (frame_id >> 1)) & 1) {
                r = (uint8_t)(30 + (r / 3));
                g = (uint8_t)(50 + (g / 3));
                b = (uint8_t)(80 + (b / 3));
            }
            dst[i] = r; dst[i+1] = g; dst[i+2] = b; dst[i+3] = 255;
        }
    }
}
