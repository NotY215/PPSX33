#include "rsx_present_queue.h"

#define QCAP 8
static rsx_present_req g_q[QCAP];
static int g_head, g_tail, g_count;
static uint64_t g_fid;

void rsx_present_queue_init(void) {
    g_head = g_tail = g_count = 0;
    g_fid = 0;
}

int rsx_present_queue_push(const rsx_present_req* req) {
    if (!req || g_count >= QCAP) return 0;
    g_q[g_tail] = *req;
    g_q[g_tail].frame_id = ++g_fid;
    g_tail = (g_tail + 1) % QCAP;
    ++g_count;
    return 1;
}

int rsx_present_queue_pop(rsx_present_req* out) {
    if (!out || g_count <= 0) return 0;
    *out = g_q[g_head];
    g_head = (g_head + 1) % QCAP;
    --g_count;
    return 1;
}

uint64_t rsx_present_queue_frame_id(void) { return g_fid; }
