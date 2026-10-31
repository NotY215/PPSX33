#include "rsx_surface_fmt.h"

rsx_color_fmt rsx_surface_fmt_from_gcm(uint32_t gcm_fmt) {
    switch (gcm_fmt & 0xFFu) {
    case 0x80: case 0x08: return RSX_FMT_A8R8G8B8;
    case 0x09: return RSX_FMT_X8R8G8B8;
    case 0x03: return RSX_FMT_R5G6B5;
    default: return RSX_FMT_A8R8G8B8;
    }
}

uint32_t rsx_surface_fmt_bpp(rsx_color_fmt f) {
    switch (f) {
    case RSX_FMT_R5G6B5: return 2;
    default: return 4;
    }
}

uint32_t rsx_surface_fmt_pitch_576p(rsx_color_fmt f) {
    return 720u * rsx_surface_fmt_bpp(f);
}
