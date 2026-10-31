#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 if a present is allowed this tick (low rate for GT 740 class GPUs). */
int  rsx_light_present_allow(void);
void rsx_light_present_note(void);
uint32_t rsx_light_present_interval_ms(void);

#ifdef __cplusplus
}
#endif
