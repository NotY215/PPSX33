#pragma once
#ifndef PPSX33_RENDER_API
#  if defined(_WIN32) && defined(PPSX33_RENDER_BUILD)
#    define PPSX33_RENDER_API __declspec(dllexport)
#  elif defined(_WIN32)
#    define PPSX33_RENDER_API __declspec(dllimport)
#  else
#    define PPSX33_RENDER_API
#  endif
#endif
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t color_offset;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t flags; /* 1=clear only, 2=guest rgba */
    uint64_t frame_id;
} rsx_present_req;

PPSX33_RENDER_API void rsx_present_queue_init(void);
PPSX33_RENDER_API int  rsx_present_queue_push(const rsx_present_req* req);
PPSX33_RENDER_API int  rsx_present_queue_pop(rsx_present_req* out);
PPSX33_RENDER_API uint64_t rsx_present_queue_frame_id(void);

#ifdef __cplusplus
}
#endif
