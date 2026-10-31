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
    uint32_t depth_offset;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t tex_offset;
    uint32_t draw_count;
    uint32_t clear_mask;
    uint64_t frame_seq;
} rsx_draw_frame;

PPSX33_RENDER_API void rsx_draw_batch_reset(void);
PPSX33_RENDER_API void rsx_draw_batch_set_color(uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h);
PPSX33_RENDER_API void rsx_draw_batch_set_tex(uint32_t offset);
PPSX33_RENDER_API void rsx_draw_batch_note_draw(void);
PPSX33_RENDER_API void rsx_draw_batch_note_clear(uint32_t mask);
PPSX33_RENDER_API const rsx_draw_frame* rsx_draw_batch_current(void);
PPSX33_RENDER_API int  rsx_draw_batch_take_frame(rsx_draw_frame* out);

#ifdef __cplusplus
}
#endif
