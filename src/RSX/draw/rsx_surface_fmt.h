#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RSX_FMT_A8R8G8B8 = 0,
    RSX_FMT_X8R8G8B8 = 1,
    RSX_FMT_R5G6B5   = 2,
    RSX_FMT_UNKNOWN  = 255
} rsx_color_fmt;

rsx_color_fmt rsx_surface_fmt_from_gcm(uint32_t gcm_fmt);
uint32_t      rsx_surface_fmt_bpp(rsx_color_fmt f);
uint32_t      rsx_surface_fmt_pitch_576p(rsx_color_fmt f);

#ifdef __cplusplus
}
#endif
