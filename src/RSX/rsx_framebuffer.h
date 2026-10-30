#pragma once
#include <cstdint>
namespace ppsx33 { namespace rsx {
struct ColorSurface {
    uint32_t offset = 0;
    uint32_t pitch = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = 0; // 0 = A8R8G8B8-ish
};
void rsx_framebuffer_init();
void rsx_framebuffer_bind_color(uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h, uint32_t format);
ColorSurface rsx_framebuffer_color();
}}
