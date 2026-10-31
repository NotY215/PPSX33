#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t color_offset;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t flags; /* 1=clear only, 2=guest rgba */
    uint64_t frame_id;
} rsx_present_req;

void rsx_present_queue_init(void);
int  rsx_present_queue_push(const rsx_present_req* req);
int  rsx_present_queue_pop(rsx_present_req* out);
uint64_t rsx_present_queue_frame_id(void);

#ifdef __cplusplus
}
#endif
