#include "rsx_shader_bind.h"

static uint32_t s_vp_off, s_vp_sz, s_fp_off, s_fp_sz;
static uint32_t s_const[32];
static int s_vp_set, s_fp_set;

void rsx_shader_bind_init(void) {
    s_vp_off = s_vp_sz = s_fp_off = s_fp_sz = 0;
    s_vp_set = s_fp_set = 0;
    for (int i = 0; i < 32; ++i) s_const[i] = 0;
}

void rsx_shader_bind_vp(uint32_t offset, uint32_t size) {
    s_vp_off = offset; s_vp_sz = size ? size : 512;
    s_vp_set = offset ? 1 : 0;
}

void rsx_shader_bind_fp(uint32_t offset, uint32_t size) {
    s_fp_off = offset; s_fp_sz = size ? size : 512;
    s_fp_set = offset ? 1 : 0;
}

void rsx_shader_bind_const(uint32_t slot, uint32_t value) {
    if (slot < 32) s_const[slot] = value;
}

uint32_t rsx_shader_bind_vp_offset(void) { return s_vp_off; }
uint32_t rsx_shader_bind_fp_offset(void) { return s_fp_off; }
uint32_t rsx_shader_bind_vp_size(void) { return s_vp_sz; }
uint32_t rsx_shader_bind_fp_size(void) { return s_fp_sz; }
int rsx_shader_bind_ready(void) { return s_vp_set && s_fp_set; }
int rsx_shader_host_program_ok(void) { return rsx_shader_bind_ready(); }
