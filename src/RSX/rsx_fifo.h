#pragma once
#include <cstdint>
namespace ppsx33 {
namespace rsx {
void rsx_fifo_init();
bool rsx_fifo_guest_put_probe(uint8_t* mem, uint64_t control_ea, uint32_t fifo_size,
                              uint32_t* out_put, uint32_t* out_get);
}
}
