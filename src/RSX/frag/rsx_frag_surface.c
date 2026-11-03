#include "rsx_frag_surface.h"

void rsx_frag_surface_reset(rsx_frag_surface* s) {
    if (!s) return;
    s->color_offset = 0;
    s->color_pitch = 0;
    s->width = 720;
    s->height = 576;
    s->format = 0;
}

void rsx_frag_surface_set_color(rsx_frag_surface* s, uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h) {
    if (!s) return;
    if (offset) s->color_offset = offset;
    if (pitch) s->color_pitch = pitch;
    if (w) s->width = w;
    if (h) s->height = h;
}

void rsx_frag_surface_from_display(rsx_frag_surface* s, uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h) {
    rsx_frag_surface_set_color(s, offset, pitch, w, h);
}

int rsx_frag_surface_valid(const rsx_frag_surface* s, uint64_t mem_size) {
    if (!s || !s->color_offset) return 0;
    uint32_t pitch = s->color_pitch ? s->color_pitch : (s->width * 4u);
    uint64_t need = (uint64_t)s->color_offset + (uint64_t)pitch * (uint64_t)(s->height ? s->height : 1u);
    return need <= mem_size;
}
