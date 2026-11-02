#include "gcm_flip_label.h"
#include <string.h>

#define GCM_MAX_BUFFERS 8
#define GCM_MAX_LABELS  256

static uint64_t s_control = 0x8000000ull;
static uint64_t s_labels  = 0x8001000ull;
static uint32_t s_flip_status = GCM_FLIP_DONE;
static uint32_t s_current_buf;
static uint32_t s_queued_buf;
static int s_flip_queued;
static uint32_t s_frame_id;
static uint32_t s_off[GCM_MAX_BUFFERS];
static uint32_t s_pitch[GCM_MAX_BUFFERS];
static uint32_t s_w[GCM_MAX_BUFFERS];
static uint32_t s_h[GCM_MAX_BUFFERS];

static uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static void wbe32(uint8_t* p, uint32_t v) {
    p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16);
    p[2]=(uint8_t)(v>>8);  p[3]=(uint8_t)v;
}
static uint32_t rd_be(uint8_t* mem, uint64_t mem_size, uint64_t ea) {
    if (!mem || ea + 4 > mem_size) return 0;
    return be32(mem + ea);
}
static void wr_be(uint8_t* mem, uint64_t mem_size, uint64_t ea, uint32_t v) {
    if (!mem || ea + 4 > mem_size) return;
    wbe32(mem + ea, v);
}

void gcm_flip_label_init(void) {
    memset(s_off, 0, sizeof(s_off));
    memset(s_pitch, 0, sizeof(s_pitch));
    s_control = 0x8000000ull;
    s_labels = 0x8001000ull;
    s_flip_status = GCM_FLIP_DONE;
    s_current_buf = s_queued_buf = 0;
    s_flip_queued = 0;
    s_frame_id = 0;
    for (int i = 0; i < GCM_MAX_BUFFERS; i++) {
        s_w[i] = 720; s_h[i] = 576; s_pitch[i] = 720 * 4;
    }
}

void gcm_set_control_addrs(uint64_t control, uint64_t labels) {
    if (control) s_control = control;
    if (labels) s_labels = labels;
}
uint64_t gcm_control_addr(void) { return s_control; }
uint64_t gcm_labels_addr(void) { return s_labels; }

void gcm_set_display_buffer(uint32_t id, uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h) {
    if (id >= GCM_MAX_BUFFERS) return;
    s_off[id] = offset;
    s_pitch[id] = pitch ? pitch : (w ? w * 4u : 720u * 4u);
    s_w[id] = w ? w : 720;
    s_h[id] = h ? h : 576;
}
uint32_t gcm_get_display_offset(uint32_t id) {
    return (id < GCM_MAX_BUFFERS) ? s_off[id] : 0;
}
uint32_t gcm_get_display_pitch(uint32_t id) {
    return (id < GCM_MAX_BUFFERS) ? s_pitch[id] : 0;
}

uint32_t gcm_get_flip_status(void) { return s_flip_status; }
void gcm_reset_flip_status(void) { s_flip_status = GCM_FLIP_QUEUED; }
void gcm_set_flip_status(uint32_t status) { s_flip_status = status; }

int gcm_queue_flip(uint32_t buffer_id) {
    if (buffer_id >= GCM_MAX_BUFFERS) buffer_id = 0;
    s_queued_buf = buffer_id;
    s_flip_queued = 1;
    s_flip_status = GCM_FLIP_QUEUED;
    return 0;
}

void gcm_complete_flip(uint8_t* mem, uint64_t mem_size) {
    if (!s_flip_queued) {
        s_flip_status = GCM_FLIP_DONE;
        return;
    }
    s_current_buf = s_queued_buf;
    s_flip_queued = 0;
    s_flip_status = GCM_FLIP_DONE;
    s_frame_id++;
    gcm_label_write(mem, mem_size, 0, s_frame_id);
}

uint32_t gcm_current_display_buffer(void) { return s_current_buf; }
uint32_t gcm_frame_id(void) { return s_frame_id; }

void gcm_label_write(uint8_t* mem, uint64_t mem_size, uint32_t index, uint32_t value) {
    if (index >= GCM_MAX_LABELS) return;
    wr_be(mem, mem_size, s_labels + (uint64_t)index * 4u, value);
}
uint32_t gcm_label_read(uint8_t* mem, uint64_t mem_size, uint32_t index) {
    if (index >= GCM_MAX_LABELS) return 0;
    return rd_be(mem, mem_size, s_labels + (uint64_t)index * 4u);
}

uint32_t gcm_ctrl_read_put(uint8_t* mem, uint64_t mem_size) {
    return rd_be(mem, mem_size, s_control + 0);
}
uint32_t gcm_ctrl_read_get(uint8_t* mem, uint64_t mem_size) {
    return rd_be(mem, mem_size, s_control + 8);
}
void gcm_ctrl_write_get(uint8_t* mem, uint64_t mem_size, uint32_t get) {
    wr_be(mem, mem_size, s_control + 8, get);
}
void gcm_ctrl_write_ref(uint8_t* mem, uint64_t mem_size, uint32_t ref) {
    wr_be(mem, mem_size, s_control + 4, ref);
}
