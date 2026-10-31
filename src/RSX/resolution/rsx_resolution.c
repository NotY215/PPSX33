#include "rsx_resolution.h"
/* Native host present: 576p (720x576) — light path for weak GPUs */
PPSX33_RES_API uint32_t rsx_res_width(void)  { return 720; }
PPSX33_RES_API uint32_t rsx_res_height(void) { return 576; }
PPSX33_RES_API uint32_t rsx_res_pitch_rgba(void) { return 720u * 4u; }
PPSX33_RES_API void rsx_res_get(uint32_t* w, uint32_t* h) {
    if (w) *w = 720;
    if (h) *h = 576;
}
