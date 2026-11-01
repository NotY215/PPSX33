#include <stdint.h>
/* NV4097 surface register touch-points used by host state (clean-room method ids). */
enum {
    NV4097_SET_SURFACE_FORMAT = 0x200,
    NV4097_SET_SURFACE_PITCH  = 0x208,
    NV4097_SET_SURFACE_COLOR_A = 0x210,
    NV4097_SET_SURFACE_ZETA   = 0x220,
    NV4097_SET_VIEWPORT_HORIZONTAL = 0xA00,
    NV4097_SET_VIEWPORT_VERTICAL   = 0xA04,
};

typedef struct {
    uint32_t color_a, pitch, format, zeta;
    uint32_t vp_x, vp_w, vp_y, vp_h;
} rsx_surface_regs;

void rsx_nv4097_apply_surface(rsx_surface_regs* s, uint32_t method, uint32_t data) {
    if (!s) return;
    switch (method) {
    case NV4097_SET_SURFACE_FORMAT: s->format = data; break;
    case NV4097_SET_SURFACE_PITCH:  s->pitch = data; break;
    case NV4097_SET_SURFACE_COLOR_A: s->color_a = data; break;
    case NV4097_SET_SURFACE_ZETA: s->zeta = data; break;
    case NV4097_SET_VIEWPORT_HORIZONTAL:
        s->vp_x = data & 0xFFFFu; s->vp_w = (data >> 16) & 0xFFFFu; break;
    case NV4097_SET_VIEWPORT_VERTICAL:
        s->vp_y = data & 0xFFFFu; s->vp_h = (data >> 16) & 0xFFFFu; break;
    default: break;
    }
}
