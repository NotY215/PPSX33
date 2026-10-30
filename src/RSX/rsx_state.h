#pragma once
#include <cstdint>
namespace ppsx33 { namespace rsx {
struct DrawState {
    uint32_t blend_enable = 0;
    uint32_t depth_test = 0;
    uint32_t depth_write = 0;
    uint32_t scissor_x = 0, scissor_y = 0, scissor_w = 0, scissor_h = 0;
    uint32_t viewport_w = 1280, viewport_h = 720;
    uint32_t draw_mode = 0; // triangles etc.
    uint32_t vertex_count = 0;
};
void rsx_state_init();
void rsx_state_set_blend(uint32_t en);
void rsx_state_set_depth(uint32_t test, uint32_t write);
void rsx_state_set_scissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void rsx_state_set_viewport(uint32_t w, uint32_t h);
void rsx_state_set_draw(uint32_t mode, uint32_t count);
DrawState rsx_state_get();
}}
