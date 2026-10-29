#pragma once
// WORK: RSX FIFO guest-PUT probe notes + depth helpers (XXVI depth).
//
// Purpose
// -------
// Guest titles advance the RSX command stream by writing the CellGcm control
// register PUT field (typically at g_gcm_control+0). Host only sees progress
// when we either:
//   1) receive an explicit ps3rt_rsx_set_put() from HLE (cellGcm*), or
//   2) actively probe guest memory for PUT updates (this depth).
//
// Observed failure mode (AIFIAR / GOW logs)
// ----------------------------------------
//   - FIFO init with fifo=0 size placeholder, then real control later
//   - PUT stays at 0 while GET is forced forward → empty stream
//   - methods_seen / unknown_methods stay low; flip still runs solid path
//
// Probe strategy (implemented in rsx_device.cpp + hle_hw_progress.cpp)
// -------------------------------------------------------------------
//   * On every ps3rt_progress_hw / flush tick:
//       put = BE32(control+0); get_guest = BE32(control+4 or +8)
//       if put != host_get → set_put(put); process_fifo(); write host GET back
//   * Cap process steps (kMax) so a stuck ring cannot spin forever
//   * Log first N PUT advances and unknown-method samples for diagnosis
//   * Never invent methods; only decode words already written by guest/HLE inject
//
// Counters (g_rsx)
// ----------------
//   methods_seen / known_methods / unknown_methods / methods_this_frame
//   packets_decoded / fifo_bytes_consumed / draws_guest_frame
//
// Where to look when PUT never advances
// -------------------------------------
//   1) g_gcm_control still 0 → cellGcmInitBody / GetControlRegister not hit
//   2) control LE vs BE mismatch → gcm_rd_ctrl_u32 endian probe
//   3) progress_hw not called from run loop
//   4) guest writing a different control block than the one we allocated
//
// This header is documentation + optional inline helpers. Active FIFO decode
// remains in rsx_device.cpp until further extraction into rsx_fifo.cpp.

#include <cstdint>

namespace ppsx33 {
namespace rsx {

// Optional compile-time toggles for depth logging (override via /D)
#ifndef PS3RT_RSX_PUT_PROBE_LOG_LIMIT
#define PS3RT_RSX_PUT_PROBE_LOG_LIMIT 24
#endif
#ifndef PS3RT_RSX_UNKNOWN_METHOD_LOG_LIMIT
#define PS3RT_RSX_UNKNOWN_METHOD_LOG_LIMIT 32
#endif

// Helper: distance on a ring of size fsz (PUT ahead of GET).
inline uint32_t fifo_ring_dist(uint32_t put, uint32_t get, uint32_t fsz) {
    if (!fsz) return 0;
    put %= fsz;
    get %= fsz;
    return (put >= get) ? (put - get) : (fsz - get + put);
}

} // namespace rsx
} // namespace ppsx33
