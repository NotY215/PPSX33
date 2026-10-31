#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef PPSX33_RES_API
#  if defined(_WIN32) && defined(PPSX33_RES_BUILD)
#    define PPSX33_RES_API __declspec(dllexport)
#  elif defined(_WIN32)
#    define PPSX33_RES_API __declspec(dllimport)
#  else
#    define PPSX33_RES_API
#  endif
#endif

PPSX33_RES_API uint32_t rsx_res_width(void);
PPSX33_RES_API uint32_t rsx_res_height(void);
PPSX33_RES_API uint32_t rsx_res_pitch_rgba(void);
PPSX33_RES_API void     rsx_res_get(uint32_t* w, uint32_t* h);

#ifdef __cplusplus
}
#endif
