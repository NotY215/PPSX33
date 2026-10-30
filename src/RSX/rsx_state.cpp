// WORK: Blend/depth/scissor/viewport/draw-mode tracking for host present.
#include "rsx_state.h"
#include <mutex>
namespace ppsx33 { namespace rsx {
static DrawState g_st;
static std::mutex g_st_mu;
void rsx_state_init() { std::lock_guard<std::mutex> lk(g_st_mu); g_st = {}; g_st.viewport_w = 1280; g_st.viewport_h = 720; }
void rsx_state_set_blend(uint32_t en) { std::lock_guard<std::mutex> lk(g_st_mu); g_st.blend_enable = en; }
void rsx_state_set_depth(uint32_t test, uint32_t write) {
    std::lock_guard<std::mutex> lk(g_st_mu); g_st.depth_test = test; g_st.depth_write = write;
}
void rsx_state_set_scissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    std::lock_guard<std::mutex> lk(g_st_mu);
    g_st.scissor_x = x; g_st.scissor_y = y; g_st.scissor_w = w; g_st.scissor_h = h;
}
void rsx_state_set_viewport(uint32_t w, uint32_t h) {
    std::lock_guard<std::mutex> lk(g_st_mu);
    if (w) g_st.viewport_w = w; if (h) g_st.viewport_h = h;
}
void rsx_state_set_draw(uint32_t mode, uint32_t count) {
    std::lock_guard<std::mutex> lk(g_st_mu);
    g_st.draw_mode = mode; g_st.vertex_count = count;
}
DrawState rsx_state_get() { std::lock_guard<std::mutex> lk(g_st_mu); return g_st; }
}}
