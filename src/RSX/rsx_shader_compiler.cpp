// WORK: Host shader compile surface (Vulkan uses render-pass clear; SCE shaders later).
#include "rsx_shader_compiler.h"
namespace ppsx33 { namespace rsx {
static int g_has = 0;
void rsx_shader_compiler_init() { g_has = 0; }
int rsx_shader_compiler_has_program() { return g_has; }
}}
