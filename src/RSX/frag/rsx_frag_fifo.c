#include "rsx_frag_fifo.h"

static uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static void wbe32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}
static uint32_t rd(uint8_t* mem, uint64_t msz, uint64_t base, uint32_t off, uint32_t fsz) {
    uint64_t ea = base + (off % fsz);
    if (ea + 4 > msz) return 0;
    return be32(mem + ea);
}

void rsx_frag_fifo_stats_clear(rsx_frag_fifo_stats* st) {
    if (st) { st->methods = st->draws = st->put = st->get = st->advanced = 0; }
}

uint32_t rsx_frag_fifo_walk(uint8_t* mem, uint64_t mem_size,
                            uint64_t fifo_base, uint32_t fifo_size,
                            uint32_t put, uint32_t get,
                            rsx_frag_fifo_stats* st) {
    if (!mem || !fifo_size) return get;
    put %= fifo_size; get %= fifo_size;
    uint32_t methods = 0, draws = 0, guard = 0;
    uint32_t start = get;
    while (get != put && guard++ < 4096u) {
        uint32_t hdr = rd(mem, mem_size, fifo_base, get, fifo_size);
        if ((hdr & 0xe0000003u) == 0x20000000u) {
            get = (hdr & 0x1ffffffcu) % fifo_size;
            methods++;
            continue;
        }
        if ((hdr & 0x3u) == 0x1u) {
            get = (hdr & 0x1ffffffcu) % fifo_size;
            methods++;
            continue;
        }
        if ((hdr & 0x3u) == 0x2u) {
            get = (get + 4) % fifo_size;
            methods++;
            continue;
        }
        /* NI method batch */
        uint32_t count = ((hdr >> 18) & 0x7ffu) + 1u;
        uint32_t method = (hdr >> 2) & 0x1fffu;
        if ((method & ~3u) == 0x1814u || (method & ~3u) == 0x1808u)
            draws++;
        get = (get + 4u + count * 4u) % fifo_size;
        methods += 1u + count;
        if (methods > 2048u) break;
    }
    if (st) {
        st->methods = methods;
        st->draws = draws;
        st->put = put;
        st->get = get;
        st->advanced = (get >= start) ? (get - start) : (fifo_size - start + get);
    }
    return get;
}

void rsx_frag_fifo_nudge_control(uint8_t* mem, uint64_t mem_size, uint64_t ctrl, uint32_t fifo_size) {
    if (!mem || !ctrl || ctrl + 12 > mem_size) return;
    if (!fifo_size) fifo_size = 0x100000u;
    uint32_t put = be32(mem + ctrl + 0) % fifo_size;
    uint32_t get = be32(mem + ctrl + 8) % fifo_size;
    if (put == get) return;
    uint32_t next = get;
    if (put > get) {
        uint32_t gap = put - get;
        next = get + (gap > 0x100u ? 0x100u : gap);
    } else {
        uint32_t gap = (fifo_size - get) + put;
        uint32_t adv = gap > 0x100u ? 0x100u : gap;
        next = (get + adv) % fifo_size;
    }
    wbe32(mem + ctrl + 8, next);
    wbe32(mem + ctrl + 4, put);
}
