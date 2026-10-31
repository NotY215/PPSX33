#include "ps3rt_be.h"
// WORK: FIFO PUT/GET probe helpers (decode path still in rsx_device.cpp).
#include "rsx_fifo.h"

namespace ppsx33 {
namespace rsx {

void rsx_fifo_init() {}

bool rsx_fifo_guest_put_probe(uint8_t* mem, uint64_t control_ea, uint32_t fifo_size,
                              uint32_t* out_put, uint32_t* out_get) {
    if (!mem || !control_ea || !fifo_size) return false;
    auto rd_be32 = [&](uint64_t ea) -> uint32_t {
        const uint8_t* p = mem + ea;
        return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
               ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    };
    auto rd_le32 = [&](uint64_t ea) -> uint32_t {
        const uint8_t* p = mem + ea;
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
               ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    };
    uint32_t put = rd_be32(control_ea + 0);
    if (put == 0) put = rd_le32(control_ea + 0);
    put %= fifo_size;
    uint32_t get = rd_be32(control_ea + 8);
    if (get == 0) get = rd_le32(control_ea + 8);
    get %= fifo_size;
    uint32_t get4 = rd_be32(control_ea + 4) % fifo_size;
    if (put != get4 && get == put && get4 != put)
        get = get4;
    if (out_put) *out_put = put;
    if (out_get) *out_get = get;
    return put != get;
}

} // namespace rsx
} // namespace ppsx33
