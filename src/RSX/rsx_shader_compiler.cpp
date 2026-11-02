#include "rsx_shader_compiler.h"
#include "rsx_shader_bind.h"
namespace ppsx33 { namespace rsx {
void rsx_shader_compiler_init() { rsx_shader_bind_init(); }
int rsx_shader_compiler_has_program() { return rsx_shader_host_program_ok(); }
}}
