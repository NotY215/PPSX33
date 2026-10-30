// WORK: Track guest shader ucode offsets (decode/SPIR-V later).
#include "rsx_shader_decoder.h"
namespace ppsx33 { namespace rsx {
static uint32_t g_last_off = 0, g_last_size = 0;
void rsx_shader_decoder_init() { g_last_off = g_last_size = 0; }
void rsx_shader_decoder_note_ucode(uint32_t offset, uint32_t size) {
    g_last_off = offset; g_last_size = size;
}
}}
