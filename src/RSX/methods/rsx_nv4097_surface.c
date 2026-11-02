#include <stdint.h>

typedef struct {
    uint32_t color_a, pitch, format, zeta;
    uint32_t vp_x, vp_w, vp_y, vp_h;
    uint32_t sc_x, sc_w, sc_y, sc_h;
} rsx_surface_regs;

void rsx_nv4097_apply_surface(void* sp, unsigned method, unsigned data) {
    rsx_surface_regs* s = (rsx_surface_regs*)sp;
    if (!s) return;
    switch (method) {
    case 0x200: s->format = data; break;
    case 0x208: case 0x22c: case 0x230: s->pitch = data; break;
    case 0x20c: case 0x210: case 0x182c: case 0x1830: s->color_a = data; break;
    case 0x220: case 0x224: s->zeta = data; break;
    case 0xa00: case 0x8c0: case 0x8c4:
        s->vp_x = data & 0xFFFFu; s->vp_w = (data >> 16) & 0xFFFFu; break;
    case 0xa04:
        s->vp_y = data & 0xFFFFu; s->vp_h = (data >> 16) & 0xFFFFu; break;
    case 0x8c8: case 0x8cc: case 0xa08:
        s->sc_x = data & 0xFFFFu; s->sc_w = (data >> 16) & 0xFFFFu; break;
    case 0xa0c:
        s->sc_y = data & 0xFFFFu; s->sc_h = (data >> 16) & 0xFFFFu; break;
    default: break;
    }
}
