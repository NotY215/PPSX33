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

typedef enum {
    RSX_PRESENT_SKIP = 0,
    RSX_PRESENT_CLEAR = 1,
    RSX_PRESENT_GUEST = 2
} rsx_present_action;

/* Decide present action from host state. */
PPSX33_RENDER_API rsx_present_action rsx_present_decide(
    int has_guest_color,
    int methods_this_frame,
    int draws_this_frame,
    uint32_t flip_count);

/* Min interval between clear-only presents (ms). */
PPSX33_RENDER_API int rsx_present_clear_interval_ms(void);

#ifdef __cplusplus
}
#endif
