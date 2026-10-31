#pragma once
#include <stdint.h>
#include "rsx_draw_batch.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Build a present request from the current draw batch (720p). */
int rsx_render_frame_enqueue_present(void);

/* Called after flip/HLE: take batch → present queue. */
int rsx_render_frame_on_flip(uint32_t color_offset, uint32_t pitch);

#ifdef __cplusplus
}
#endif
