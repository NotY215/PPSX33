#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Track repeated sysmodule import hits for diagnostics. */
void hle_sysmodule_note_call(uint32_t nid);
uint32_t hle_sysmodule_call_count(uint32_t nid);
#ifdef __cplusplus
}
#endif
