#pragma once
#include <stdint.h>
#include "rsx_frag_surface.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Apply a single decoded NI method word to surface state (redesigned method table slice). */
void rsx_frag_method_apply(rsx_frag_surface* s, uint32_t method, uint32_t arg);
#ifdef __cplusplus
}
#endif
