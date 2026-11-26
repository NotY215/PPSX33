#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rsx_sw_tex {
    uint32_t offset;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t format;
} rsx_sw_tex;

typedef struct rsx_sw_state {
    uint32_t color_offset;
    uint32_t color_pitch;
    uint32_t width;
    uint32_t height;
    uint32_t clear_color;
    uint32_t va_offset[8];
    uint32_t va_fmt[8];
    rsx_sw_tex tex0;
    int begin_mode;
} rsx_sw_state;

void rsx_sw_state_init(rsx_sw_state* s);
void rsx_sw_set_color(rsx_sw_state* s, uint32_t off, uint32_t pitch, uint32_t w, uint32_t h);
void rsx_sw_set_tex0(rsx_sw_state* s, uint32_t off, uint32_t pitch, uint32_t w, uint32_t h, uint32_t fmt);
void rsx_sw_set_va(rsx_sw_state* s, int idx, uint32_t off, uint32_t fmt);
void rsx_sw_clear(rsx_sw_state* s, uint8_t* mem, size_t msz);
void rsx_sw_draw_arrays(rsx_sw_state* s, uint8_t* mem, size_t msz, uint32_t arg, int draw_id);

#ifdef __cplusplus
}
#endif
