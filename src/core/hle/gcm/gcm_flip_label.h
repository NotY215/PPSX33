#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum {
    GCM_FLIP_DONE   = 0,
    GCM_FLIP_QUEUED = 1
};

void     gcm_flip_label_init(void);
void     gcm_set_control_addrs(uint64_t control, uint64_t labels);
uint64_t gcm_control_addr(void);
uint64_t gcm_labels_addr(void);

void     gcm_set_display_buffer(uint32_t id, uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h);
uint32_t gcm_get_display_offset(uint32_t id);
uint32_t gcm_get_display_pitch(uint32_t id);

uint32_t gcm_get_flip_status(void);
void     gcm_reset_flip_status(void);
void     gcm_set_flip_status(uint32_t status);

int      gcm_queue_flip(uint32_t buffer_id);
void     gcm_complete_flip(uint8_t* mem, uint64_t mem_size);
uint32_t gcm_current_display_buffer(void);
uint32_t gcm_frame_id(void);

void     gcm_label_write(uint8_t* mem, uint64_t mem_size, uint32_t index, uint32_t value);
uint32_t gcm_label_read(uint8_t* mem, uint64_t mem_size, uint32_t index);

/* Control layout: PUT@0, REF@4, GET@8 (BE). */
uint32_t gcm_ctrl_read_put(uint8_t* mem, uint64_t mem_size);
uint32_t gcm_ctrl_read_get(uint8_t* mem, uint64_t mem_size);
void     gcm_ctrl_write_get(uint8_t* mem, uint64_t mem_size, uint32_t get);
void     gcm_ctrl_write_ref(uint8_t* mem, uint64_t mem_size, uint32_t ref);

#ifdef __cplusplus
}
#endif
