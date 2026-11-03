#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rsx_frag_surface {
    uint32_t color_offset;
    uint32_t color_pitch;
    uint32_t width;
    uint32_t height;
    uint32_t format;
} rsx_frag_surface;
void rsx_frag_surface_reset(rsx_frag_surface* s);
void rsx_frag_surface_set_color(rsx_frag_surface* s, uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h);
void rsx_frag_surface_from_display(rsx_frag_surface* s, uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h);
int  rsx_frag_surface_valid(const rsx_frag_surface* s, uint64_t mem_size);
#ifdef __cplusplus
}
#endif
