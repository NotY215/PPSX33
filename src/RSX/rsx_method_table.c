#include "rsx_method_table.h"

/*
 * Method ids are byte offsets as used by our FIFO decode path (not reg/4).
 * Catalog informed by public NV4097 / GCM method lists; values rewritten for PPSX33.
 */
PPSX33_RSX_API rsx_method_kind rsx_classify_method(uint32_t method) {
    switch (method) {
    case 0x50: case 0x54: case 0x100: case 0x0:
        return RSX_M_NOP;

    case 0x200: /* SET_SURFACE_FORMAT */
        return RSX_M_SURFACE_FORMAT;
    case 0x208: case 0x22c: case 0x230:
        return RSX_M_SURFACE_PITCH;
    case 0x20c: case 0x210: case 0x214: case 0x218:
    case 0x182c: case 0x1830: case 0x198: case 0x1a00:
        return RSX_M_SURFACE_COLOR_OFFSET;
    case 0x220: case 0x224:
        return RSX_M_SURFACE_ZETA_OFFSET;
    case 0x8: case 0xc: case 0x10:
        return RSX_M_SURFACE_CLIP;

    case 0xa00: case 0xa04: case 0x8c0: case 0x8c4:
        return RSX_M_VIEWPORT;
    case 0x8c8: case 0x8cc: case 0xa08: case 0xa0c:
        return RSX_M_SCISSOR;

    case 0x1d88: case 0x1840: case 0x1844: case 0x1a04: case 0x1a08:
        return RSX_M_TEXTURE_OFFSET;
    case 0x1a0c: case 0x1a10: case 0x1848:
        return RSX_M_TEXTURE_FORMAT;

    case 0x1808: /* BEGIN_END */
        return RSX_M_BEGIN_END;
    case 0x1814: case 0x1824: case 0x1800: case 0x1804:
    case 0x180c: case 0x1810:
        return RSX_M_DRAW;
    case 0x1d8c: case 0x1dac: case 0x1d94: case 0x1d9c:
        return RSX_M_CLEAR;

    case 0x64: case 0x68: case 0x6c:
        return RSX_M_SEMAPHORE;
    case 0x1d00: case 0x1d04: case 0x1d08:
        return RSX_M_TRANSFORM;

    /* FIFO control-ish / flip hints seen in streams */
    case 0xe000: case 0xe004: case 0x1000:
        return RSX_M_FLIP_HINT;

    default:
        if (method >= 0x1a00 && method < 0x1b00) return RSX_M_TEXTURE_OFFSET;
        if (method >= 0x1800 && method <= 0x1c00) return RSX_M_DRAW;
        if (method <= 0x200) return RSX_M_NOP;
        return RSX_M_UNKNOWN;
    }
}

PPSX33_RSX_API const char* rsx_method_name(rsx_method_kind k) {
    switch (k) {
    case RSX_M_NOP: return "nop";
    case RSX_M_SURFACE_FORMAT: return "surface_format";
    case RSX_M_SURFACE_PITCH: return "surface_pitch";
    case RSX_M_SURFACE_COLOR_OFFSET: return "surface_color_offset";
    case RSX_M_SURFACE_ZETA_OFFSET: return "surface_zeta_offset";
    case RSX_M_SURFACE_CLIP: return "surface_clip";
    case RSX_M_VIEWPORT: return "viewport";
    case RSX_M_SCISSOR: return "scissor";
    case RSX_M_TEXTURE_OFFSET: return "texture_offset";
    case RSX_M_TEXTURE_FORMAT: return "texture_format";
    case RSX_M_DRAW: return "draw";
    case RSX_M_CLEAR: return "clear";
    case RSX_M_BEGIN_END: return "begin_end";
    case RSX_M_SEMAPHORE: return "semaphore";
    case RSX_M_FLIP_HINT: return "flip_hint";
    case RSX_M_TRANSFORM: return "transform";
    case RSX_M_JUMP_CALL: return "jump_call";
    default: return "unknown";
    }
}

PPSX33_RSX_API void rsx_fifo_decode_header(uint32_t hdr, uint32_t* out_method, uint32_t* out_count) {
    /* GCM-style: method in low bits, count in high — match existing host path */
    uint32_t method = (hdr & 0x3ffcu);
    uint32_t count = ((hdr >> 18) & 0x7ffu);
    if (count == 0) count = 1;
    if (out_method) *out_method = method;
    if (out_count) *out_count = count;
}

PPSX33_RSX_API int rsx_method_is_drawish(rsx_method_kind k) {
    return k == RSX_M_DRAW || k == RSX_M_CLEAR || k == RSX_M_BEGIN_END || k == RSX_M_FLIP_HINT;
}
