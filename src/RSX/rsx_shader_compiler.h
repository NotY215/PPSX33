#pragma once
#include <cstdint>
namespace ppsx33 { namespace rsx {
// Host shader path is Vulkan clear/blit for now; SCE VP/FP compile is future.
void rsx_shader_compiler_init();
int rsx_shader_compiler_has_program();
}}
