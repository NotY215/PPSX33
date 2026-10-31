// WORK: Texture unit offsets/format for host sample path.
#include "rsx_texture.h"
#include <mutex>
namespace ppsx33 { namespace rsx {
static TexUnit g_tex[8];
static std::mutex g_tex_mu;
void rsx_texture_init() {
    std::lock_guard<std::mutex> lk(g_tex_mu);
    for (auto& t : g_tex) t = {};
}
void rsx_texture_bind(uint32_t unit, uint32_t offset, uint32_t format, uint32_t w, uint32_t h, uint32_t pitch) {
    if (unit >= 8) return;
    std::lock_guard<std::mutex> lk(g_tex_mu);
    g_tex[unit].offset = offset;
    g_tex[unit].format = format;
    g_tex[unit].width = w ? w : 1280;
    g_tex[unit].height = h ? h : 720;
    g_tex[unit].pitch = pitch ? pitch : ((g_tex[unit].width) * 4u);
}
TexUnit rsx_texture_get(uint32_t unit) {
    if (unit >= 8) return {};
    std::lock_guard<std::mutex> lk(g_tex_mu);
    return g_tex[unit];
}
}}
