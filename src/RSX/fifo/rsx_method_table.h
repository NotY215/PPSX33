#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RSX_M_NOP = 0,
    RSX_M_SURFACE_COLOR_OFFSET = 1,
    RSX_M_SURFACE_PITCH = 2,
    RSX_M_SURFACE_CLIP = 3,
    RSX_M_TEXTURE_OFFSET = 4,
    RSX_M_DRAW = 5,
    RSX_M_CLEAR = 6,
    RSX_M_FLIP_HINT = 7,
    RSX_M_UNKNOWN = 255
} rsx_method_kind;

rsx_method_kind rsx_classify_method(uint32_t method_byte_offset);
const char*     rsx_method_name(rsx_method_kind k);

/* Decode one FIFO word pair: method header style (count<<18)|method */
void rsx_fifo_decode_header(uint32_t hdr, uint32_t* out_method, uint32_t* out_count);

#ifdef __cplusplus
}
#endif
