#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void rsx_shader_bind_init(void);
void rsx_shader_bind_vp(uint32_t offset, uint32_t size);
void rsx_shader_bind_fp(uint32_t offset, uint32_t size);
void rsx_shader_bind_const(uint32_t slot, uint32_t value);
uint32_t rsx_shader_bind_vp_offset(void);
uint32_t rsx_shader_bind_fp_offset(void);
uint32_t rsx_shader_bind_vp_size(void);
uint32_t rsx_shader_bind_fp_size(void);
int rsx_shader_bind_ready(void);
/* Host placeholder: returns 1 when both VP/FP offsets seen (SPIR-V path later). */
int rsx_shader_host_program_ok(void);

#ifdef __cplusplus
}
#endif
