#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Minimal SPURS job submit — records entry for SPU path diagnostics. */
int ps3rt_spurs_submit_job(uint32_t entry, uint32_t arg0, uint32_t arg1);
#ifdef __cplusplus
}
#endif
