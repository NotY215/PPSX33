// WORK: FIFO PUT/GET method decode + guest-PUT probe (XXVI depth).
// Split surface — active process_fifo path remains in rsx_device.cpp until
// further extraction. This TU owns init + the guest-control probe helper.

#include "rsx_fifo.h"
#include <cstdio>

namespace ppsx33 {
namespace rsx {

void rsx_fifo_init() {
    // Marker so logs show the depth surface is linked.
    static int once = 0;
    if (!once) {
        once = 1;
        // device init already logs fifo setup via ps3rt_rsx_init
    }
}

bool rsx_fifo_guest_put_probe(uint8_t* mem, uint64_t control_ea, uint32_t fifo_size,
                              uint32_t* out_put, uint32_t* out_get) {
    if (!mem || !control_ea || !fifo_size) return false;
    // Control layout (CellGcm): +0 PUT, +4 REF, +8 GET  (big-endian guest)
    auto rd_be32 = [&](uint64_t ea) -> uint32_t {
        const uint8_t* p = mem + ea;
        return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
               ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    };
    uint32_t put = rd_be32(control_ea + 0) % fifo_size;
    uint32_t get = rd_be32(control_ea + 8) % fifo_size;
    // Some firmwares mirror GET at +4; prefer +4 when +8 matches PUT but +4 differs
    uint32_t get4 = rd_be32(control_ea + 4) % fifo_size;
    if (put != get4 && get == put && get4 != put)
        get = get4;
    if (out_put) *out_put = put;
    if (out_get) *out_get = get;
    return put != get;
}

} // namespace rsx
} // namespace ppsx33
