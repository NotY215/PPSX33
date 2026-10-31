#include "rsx_state.h"
#include <mutex>

namespace ppsx33 { namespace rsx {

static DrawState g_ds;
static std::mutex g_ds_mu;

void rsx_state_init() {
    std::lock_guard<std::mutex> lk(g_ds_mu);
    g_ds = DrawState{};
    g_ds.viewport_w = 720;
    g_ds.viewport_h = 576;
    g_ds.scissor_w = 720;
    g_ds.scissor_h = 576;
}

void rsx_state_set_blend(uint32_t en) {
    std::lock_guard<std::mutex> lk(g_ds_mu);
    g_ds.blend_enable = en;
}

void rsx_state_set_depth(uint32_t test, uint32_t write) {
    std::lock_guard<std::mutex> lk(g_ds_mu);
    g_ds.depth_test = test;
    g_ds.depth_write = write;
}

void rsx_state_set_scissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    std::lock_guard<std::mutex> lk(g_ds_mu);
    g_ds.scissor_x = x;
    g_ds.scissor_y = y;
    g_ds.scissor_w = w ? w : 720;
    g_ds.scissor_h = h ? h : 576;
}

void rsx_state_set_viewport(uint32_t w, uint32_t h) {
    std::lock_guard<std::mutex> lk(g_ds_mu);
    g_ds.viewport_w = w ? w : 720;
    g_ds.viewport_h = h ? h : 576;
}

void rsx_state_set_draw(uint32_t mode, uint32_t count) {
    std::lock_guard<std::mutex> lk(g_ds_mu);
    g_ds.draw_mode = mode;
    g_ds.vertex_count = count;
}

DrawState rsx_state_get() {
    std::lock_guard<std::mutex> lk(g_ds_mu);
    return g_ds;
}

}} // namespace
