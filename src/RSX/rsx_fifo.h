#pragma once
// WORK: FIFO PUT/GET method decode + guest-PUT probe surface.
// Active process_fifo still lives in rsx_device.cpp; this header exposes
// the depth helpers and notes include so callers share one path layout.

#include "rsx_fifo_guest_put_notes.h"

namespace ppsx33 {
namespace rsx {

void rsx_fifo_init();

// Optional: re-probe guest control registers (PUT/GET) when mem + control EA known.
// Returns true if PUT advanced relative to previous host view.
bool rsx_fifo_guest_put_probe(uint8_t* mem, uint64_t control_ea, uint32_t fifo_size,
                              uint32_t* out_put, uint32_t* out_get);

} // namespace rsx
} // namespace ppsx33
