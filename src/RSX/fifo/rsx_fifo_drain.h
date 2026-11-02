#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Advance control GET toward PUT (control+0 PUT, control+4 GET, both BE). */
void rsx_fifo_drain_control(uint8_t* mem, uint64_t mem_size, uint64_t ctrl, uint32_t fifo_size);

/*
 * Walk ring [get,put) at fifo_base, classify methods, advance GET by consumed words.
 * out_draws increments for draw/clear/begin_end-ish methods.
 * Returns new GET offset.
 */
uint32_t rsx_fifo_drain_walk(uint8_t* mem, uint64_t mem_size,
                             uint64_t fifo_base, uint32_t fifo_size,
                             uint32_t put, uint32_t get,
                             uint32_t* out_methods, uint32_t* out_draws);

#ifdef __cplusplus
}
#endif
