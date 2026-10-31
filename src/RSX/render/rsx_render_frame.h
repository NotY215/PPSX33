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
#include "rsx_draw_batch.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Build a present request from the current draw batch (720p). */
PPSX33_RENDER_API int rsx_render_frame_enqueue_present(void);

/* Called after flip/HLE: take batch → present queue. */
PPSX33_RENDER_API int rsx_render_frame_on_flip(uint32_t color_offset, uint32_t pitch);

#ifdef __cplusplus
}
#endif
