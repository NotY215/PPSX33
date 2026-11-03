#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void rsx_frag_flip_complete_label(uint8_t* mem, uint64_t mem_size, uint64_t labels_ea, uint32_t frame_id);
#ifdef __cplusplus
}
#endif
