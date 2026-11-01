#pragma once
#ifndef PPSX33_RSX_API
#  if defined(_WIN32) && defined(PPSX33_RSX_BUILD)
#    define PPSX33_RSX_API __declspec(dllexport)
#  elif defined(_WIN32)
#    define PPSX33_RSX_API __declspec(dllimport)
#  else
#    define PPSX33_RSX_API
#  endif
#endif
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

/* declspec must precede the return type for MSVC /TC */
PPSX33_RSX_API rsx_method_kind rsx_classify_method(uint32_t method_byte_offset);
PPSX33_RSX_API const char*     rsx_method_name(rsx_method_kind k);
PPSX33_RSX_API void            rsx_fifo_decode_header(uint32_t hdr, uint32_t* out_method, uint32_t* out_count);

#ifdef __cplusplus
}
#endif
