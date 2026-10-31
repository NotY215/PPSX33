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

/* Returns 1 if a present is allowed this tick (low rate for GT 740 class GPUs). */
PPSX33_RENDER_API int  rsx_light_present_allow(void);
PPSX33_RENDER_API void rsx_light_present_note(void);
PPSX33_RENDER_API uint32_t rsx_light_present_interval_ms(void);

#ifdef __cplusplus
}
#endif
