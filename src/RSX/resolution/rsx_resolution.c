#include "rsx_resolution.h"

static uint32_t s_w = 720;
static uint32_t s_h = 560; /* default 560p */

PPSX33_RES_API uint32_t rsx_res_width(void)  { return s_w; }
PPSX33_RES_API uint32_t rsx_res_height(void) { return s_h; }
PPSX33_RES_API uint32_t rsx_res_pitch_rgba(void) { return s_w * 4u; }
PPSX33_RES_API void rsx_res_get(uint32_t* w, uint32_t* h) {
    if (w) *w = s_w;
    if (h) *h = s_h;
}
PPSX33_RES_API void rsx_res_set(uint32_t w, uint32_t h) {
    if (w >= 320 && w <= 3840) s_w = w;
    if (h >= 240 && h <= 2160) s_h = h;
}
