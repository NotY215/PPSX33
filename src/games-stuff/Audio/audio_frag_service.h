#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Extra host audio service step (ring keep-alive) — call from align tick. */
void audio_frag_service_tick(void);
#ifdef __cplusplus
}
#endif
