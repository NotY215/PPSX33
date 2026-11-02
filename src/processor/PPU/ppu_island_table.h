#pragma once
#include <stdint.h>

/* Shared sticky / null-island table + ranked escape helper.
 * Header-only so host DLL and generated game objs both see the same policy. */

static inline int ppu_is_null_island(uint64_t p) {
    if (!p) return 1;
    if (p < 0x10100ull) return 1;
    if (p == 0x199c78ull || p == 0x105e8ull || p == 0x10218ull) return 1;
    if (p == 0x33723cull) return 1;
    if (p == 0x336660ull || p == 0x336750ull || p == 0x331c30ull || p == 0x331ba8ull) return 1;
    if (p == 0x33b1a0ull || p == 0x33aec8ull) return 1;
    if (p == 0x18000ull || p == 0x10000ull) return 1;
    if (p >= 0x336000ull && p < 0x338000ull) return 1;
    if (p == 0x430000ull) return 1;
    /* Soft parks seen as ping-pong targets */
    if (p >= 0x10000ull && p < 0x20000ull) return 1;
    return 0;
}

/* Prefer candidates that are translated, not islands, not banned. */
static inline uint64_t ppu_rank_escape(
    uint64_t lr, uint64_t alt, uint64_t last_good, uint64_t stack0, uint64_t stack1,
    int (*in_chunks)(uint64_t),
    int (*banned)(uint64_t))
{
    uint64_t cands[6] = { lr, last_good, stack0, stack1, alt, 0 };
    for (int i = 0; i < 6; ++i) {
        uint64_t p = cands[i];
        if (!p) continue;
        if (ppu_is_null_island(p)) continue;
        if (banned && banned(p)) continue;
        if (in_chunks && !in_chunks(p)) continue;
        return p;
    }
    /* Second pass: allow non-chunk if not island/banned */
    for (int i = 0; i < 6; ++i) {
        uint64_t p = cands[i];
        if (!p || ppu_is_null_island(p)) continue;
        if (banned && banned(p)) continue;
        return p;
    }
    return 0;
}

static inline int ppu_is_pingpong_pair(uint64_t a, uint64_t b) {
    if (!a || !b || a == b) return 0;
    if (ppu_is_null_island(a) && ppu_is_null_island(b)) return 1;
    if ((a >= 0x336000ull && a < 0x338000ull) && (b >= 0x10000ull && b < 0x20000ull)) return 1;
    if ((b >= 0x336000ull && b < 0x338000ull) && (a >= 0x10000ull && a < 0x20000ull)) return 1;
    return 0;
}
