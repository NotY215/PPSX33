#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 if ea looks like import trampoline island (0x39800000). */
int ps3rt_is_import_pc(uint64_t pc);

/* Resolve NID preferring static bind table, then r11-style maybe_nid. */
uint32_t ps3rt_resolve_import_nid(uint64_t pc, uint64_t lr, uint32_t maybe_r11);

#ifdef __cplusplus
}
#endif
