#include "audio_frag_service.h"

/* Forward to existing host audio tick when linked into ps3rt */
#ifdef _MSC_VER
void ps3rt_audio_tick(void);
#else
void ps3rt_audio_tick(void);
#endif

void audio_frag_service_tick(void) {
    ps3rt_audio_tick();
}
