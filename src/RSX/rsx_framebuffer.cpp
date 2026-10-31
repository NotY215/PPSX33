// WORK: Guest color/depth surface binding for present path.
#include "rsx_framebuffer.h"
#include <mutex>
namespace ppsx33 { namespace rsx {
static ColorSurface g_color;
static std::mutex g_fb_mu;
void rsx_framebuffer_init() {
    std::lock_guard<std::mutex> lk(g_fb_mu);
    g_color = {};
}
void rsx_framebuffer_bind_color(uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h, uint32_t format) {
    std::lock_guard<std::mutex> lk(g_fb_mu);
    g_color.offset = offset;
    g_color.pitch = pitch ? pitch : (w ? ((w * 4u + 63u) & ~63u) : 0);
    if (w) g_color.width = 1280;
    if (h) g_color.height = 720;
    g_color.format = format;
}
ColorSurface rsx_framebuffer_color() {
    std::lock_guard<std::mutex> lk(g_fb_mu);
    return g_color;
}
}}
