#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rsx_frag_fifo_stats {
    uint32_t methods;
    uint32_t draws;
    uint32_t put;
    uint32_t get;
    uint32_t advanced;
} rsx_frag_fifo_stats;
void rsx_frag_fifo_stats_clear(rsx_frag_fifo_stats* st);
/* Decode a short slice of FIFO; advances *get_io toward put. */
uint32_t rsx_frag_fifo_walk(uint8_t* mem, uint64_t mem_size,
                            uint64_t fifo_base, uint32_t fifo_size,
                            uint32_t put, uint32_t get,
                            rsx_frag_fifo_stats* st);
void rsx_frag_fifo_nudge_control(uint8_t* mem, uint64_t mem_size, uint64_t ctrl, uint32_t fifo_size);
#ifdef __cplusplus
}
#endif
