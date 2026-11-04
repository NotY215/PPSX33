#include "rsx_frag_method_apply.h"

void rsx_frag_method_apply(rsx_frag_surface* s, uint32_t method, uint32_t arg) {
    if (!s) return;
    method &= ~3u;
    switch (method) {
    case 0x1820: /* SET_SURFACE_PITCH_A-ish */
        if (arg) s->color_pitch = arg & 0xffffu;
        break;
    case 0x1828: /* SET_SURFACE_COLOR_OFFSET */
        if (arg) s->color_offset = arg;
        break;
    case 0x1830:
        if (arg) { s->width = arg & 0xffffu; s->height = (arg >> 16) & 0xffffu; }
        break;
    case 0x01c: /* NV406E SET_REFERENCE — ignore */
        break;
    default:
        break;
    }
}
