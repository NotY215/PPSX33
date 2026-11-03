#include "rsx_frag_flip.h"

static void wbe32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

void rsx_frag_flip_complete_label(uint8_t* mem, uint64_t mem_size, uint64_t labels_ea, uint32_t frame_id) {
    if (!mem || !labels_ea) return;
    /* Label 0 often used as flip notify; write frame id big-endian */
    if (labels_ea + 4 <= mem_size)
        wbe32(mem + labels_ea, frame_id);
    if (labels_ea + 16 <= mem_size)
        wbe32(mem + labels_ea + 12, frame_id);
}
