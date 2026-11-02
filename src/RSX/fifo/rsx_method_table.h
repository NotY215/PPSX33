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
    RSX_M_SURFACE_FORMAT = 1,
    RSX_M_SURFACE_PITCH = 2,
    RSX_M_SURFACE_COLOR_OFFSET = 3,
    RSX_M_SURFACE_ZETA_OFFSET = 4,
    RSX_M_SURFACE_CLIP = 5,
    RSX_M_VIEWPORT = 6,
    RSX_M_SCISSOR = 7,
    RSX_M_TEXTURE_OFFSET = 8,
    RSX_M_TEXTURE_FORMAT = 9,
    RSX_M_DRAW = 10,
    RSX_M_CLEAR = 11,
    RSX_M_BEGIN_END = 12,
    RSX_M_SEMAPHORE = 13,
    RSX_M_FLIP_HINT = 14,
    RSX_M_TRANSFORM = 15,
    RSX_M_JUMP_CALL = 16,
    RSX_M_UNKNOWN = 255
} rsx_method_kind;

PPSX33_RSX_API rsx_method_kind rsx_classify_method(uint32_t method_byte_offset);
PPSX33_RSX_API const char*     rsx_method_name(rsx_method_kind k);
PPSX33_RSX_API void            rsx_fifo_decode_header(uint32_t hdr, uint32_t* out_method, uint32_t* out_count);
/* Returns 1 if method is a draw/clear trigger for host present assist. */
PPSX33_RSX_API int             rsx_method_is_drawish(rsx_method_kind k);

#ifdef __cplusplus
}
#endif
