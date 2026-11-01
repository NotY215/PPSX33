#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Defined in spu_interpreter.cpp */
int ps3rt_spurs_submit_job(uint32_t entry, uint32_t prio, uint32_t workload_id);
#ifdef __cplusplus
}
#endif
