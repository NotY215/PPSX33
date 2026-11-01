#include "rsx_fifo_drain.h"
#include <string.h>

/* Redesigned drain kernel: control+0 = PUT, control+4 = GET (BE). */
static uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static void wbe32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

void rsx_fifo_drain_control(uint8_t* mem, uint64_t mem_size, uint64_t ctrl, uint32_t fifo_size) {
    if (!mem || !ctrl || ctrl + 8 > mem_size) return;
    if (!fifo_size) fifo_size = 0x100000u;
    uint32_t put = be32(mem + ctrl) % fifo_size;
    uint32_t get = be32(mem + ctrl + 4) % fifo_size;
    if (put == get) return;
    /* Advance published GET toward PUT in 128-byte steps (fetch granularity model). */
    uint32_t step = 0x80u;
    uint32_t next = get;
    if (put > get) {
        uint32_t gap = put - get;
        next = get + (gap > step ? step : gap);
    } else {
        uint32_t gap = (fifo_size - get) + put;
        uint32_t adv = gap > step ? step : gap;
        next = (get + adv) % fifo_size;
    }
    wbe32(mem + ctrl + 4, next);
}
