#pragma once
#include <stdint.h>
#include "rsx_frag_surface.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rsx_frag_present_req {
    uint32_t color_offset;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t flags; /* 2 = guest rgba */
} rsx_frag_present_req;
void rsx_frag_present_from_surface(rsx_frag_present_req* out, const rsx_frag_surface* s);
void rsx_frag_present_fill_pattern(uint8_t* mem, uint64_t mem_size,
                                   uint32_t offset, uint32_t w, uint32_t h, uint32_t frame_id);
#ifdef __cplusplus
}
#endif
