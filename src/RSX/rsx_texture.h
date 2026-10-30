#pragma once
#include <cstdint>
namespace ppsx33 { namespace rsx {
struct TexUnit {
    uint32_t offset = 0;
    uint32_t format = 0;
    uint32_t width = 0, height = 0;
    uint32_t pitch = 0;
};
void rsx_texture_init();
void rsx_texture_bind(uint32_t unit, uint32_t offset, uint32_t format, uint32_t w, uint32_t h, uint32_t pitch);
TexUnit rsx_texture_get(uint32_t unit);
}}
