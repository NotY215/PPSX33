#include "rsx_method_table.h"

rsx_method_kind rsx_classify_method(uint32_t method) {
    /* method is byte offset style as seen in our FIFO path */
    switch (method) {
    /* NV4097 SET_SURFACE_COLOR_AOFFSET / BOFFSET and common GCM offsets */
    case 0x182c: case 0x1830: case 0x198: case 0x1a00:
    case 0x20c: case 0x210: case 0x214: case 0x218:
        return RSX_M_SURFACE_COLOR_OFFSET;
    case 0x22c: case 0x230: /* pitch */
        return RSX_M_SURFACE_PITCH;
    case 0x1d88: case 0x1840: case 0x1844: case 0x1a04: case 0x1a08:
        return RSX_M_TEXTURE_OFFSET;
    case 0x8c0: case 0x8c4: case 0x8c8: case 0x8cc:
    case 0x8: case 0xc: case 0x10:
        return RSX_M_SURFACE_CLIP;
    case 0x1814: case 0x1824: case 0x1808: case 0x1800: case 0x1804:
        return RSX_M_DRAW;
    case 0x1d8c: case 0x1dac: case 0x1d94:
        return RSX_M_CLEAR;
    case 0x50: case 0x54: case 0x100:
        return RSX_M_NOP;
    default:
        if (method >= 0x1800 && method <= 0x1C00) return RSX_M_DRAW;
        if (method <= 0x200) return RSX_M_NOP;
        return RSX_M_UNKNOWN;
    }
}

const char* rsx_method_name(rsx_method_kind k) {
    switch (k) {
    case RSX_M_NOP: return "nop";
    case RSX_M_SURFACE_COLOR_OFFSET: return "surface_color_offset";
    case RSX_M_SURFACE_PITCH: return "surface_pitch";
    case RSX_M_SURFACE_CLIP: return "surface_clip";
    case RSX_M_TEXTURE_OFFSET: return "texture_offset";
    case RSX_M_DRAW: return "draw";
    case RSX_M_CLEAR: return "clear";
    case RSX_M_FLIP_HINT: return "flip_hint";
    default: return "unknown";
    }
}

void rsx_fifo_decode_header(uint32_t hdr, uint32_t* out_method, uint32_t* out_count) {
    uint32_t method = (hdr & 0x1FFF) << 2;
    uint32_t count = (hdr >> 18) & 0x7FF;
    if (!count) count = 1;
    if (out_method) *out_method = method;
    if (out_count) *out_count = count;
}
