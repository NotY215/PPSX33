#include "rsx_shader_decoder.h"
#include "rsx_shader_bind.h"
namespace ppsx33 { namespace rsx {
static uint32_t g_last_off, g_last_size;
void rsx_shader_decoder_init() {
    g_last_off = g_last_size = 0;
    rsx_shader_bind_init();
}
void rsx_shader_decoder_note_ucode(uint32_t offset, uint32_t size) {
    g_last_off = offset;
    g_last_size = size;
    /* Heuristic: first bind VP, second FP when size looks like program. */
    static int n = 0;
    if (!offset) return;
    if ((n++ & 1) == 0) rsx_shader_bind_vp(offset, size);
    else rsx_shader_bind_fp(offset, size);
}
}}
