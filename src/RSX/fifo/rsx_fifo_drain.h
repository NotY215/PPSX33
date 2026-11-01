#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Drain guest control PUT->GET into host FIFO consumer. */
void rsx_fifo_drain_control(uint8_t* mem, uint64_t mem_size, uint64_t ctrl, uint32_t fifo_size);
#ifdef __cplusplus
}
#endif
