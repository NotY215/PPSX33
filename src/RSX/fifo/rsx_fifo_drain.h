#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Control: PUT@+0, REF@+4, GET@+8 (big-endian). */
void rsx_fifo_drain_control(uint8_t* mem, uint64_t mem_size, uint64_t ctrl, uint32_t fifo_size);

uint32_t rsx_fifo_drain_walk(uint8_t* mem, uint64_t mem_size,
                             uint64_t fifo_base, uint32_t fifo_size,
                             uint32_t put, uint32_t get,
                             uint32_t* out_methods, uint32_t* out_draws);

/*
 * Read PUT/GET from control, walk FIFO, write new GET back to control+8.
 * Returns 1 if any work done.
 */
int rsx_fifo_drain_publish(uint8_t* mem, uint64_t mem_size,
                           uint64_t ctrl, uint64_t fifo_base, uint32_t fifo_size,
                           uint32_t* out_methods, uint32_t* out_draws);

#ifdef __cplusplus
}
#endif
