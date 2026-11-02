#include "rsx_fifo_drain.h"
#include "rsx_method_table.h"

static uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static void wbe32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}
static uint32_t rd_fifo_u32(uint8_t* mem, uint64_t mem_size, uint64_t base, uint32_t off, uint32_t fsz) {
    uint64_t ea = base + (off % fsz);
    if (ea + 4 > mem_size) return 0;
    return be32(mem + ea);
}

void rsx_fifo_drain_control(uint8_t* mem, uint64_t mem_size, uint64_t ctrl, uint32_t fifo_size) {
    if (!mem || !ctrl || ctrl + 12 > mem_size) return;
    if (!fifo_size) fifo_size = 0x100000u;
    uint32_t put = be32(mem + ctrl + 0) % fifo_size;
    uint32_t get = be32(mem + ctrl + 8) % fifo_size;
    uint32_t ref = be32(mem + ctrl + 4) % fifo_size;
    if (get == put && ref != put) get = ref;
    if (put == get) return;
    uint32_t step = 0x80u;
    uint32_t next = get;
    if (put > get) {
        uint32_t gap = put - get;
        next = get + (gap > step ? step : gap);
    } else {
        uint32_t gap = (fifo_size - get) + put;
        uint32_t adv = gap > step ? step : gap;
        next = (get + adv) % fifo_size;
    }
    wbe32(mem + ctrl + 8, next);
    /* Keep REF near PUT so titles that poll REF progress. */
    wbe32(mem + ctrl + 4, put);
}

uint32_t rsx_fifo_drain_walk(uint8_t* mem, uint64_t mem_size,
                             uint64_t fifo_base, uint32_t fifo_size,
                             uint32_t put, uint32_t get,
                             uint32_t* out_methods, uint32_t* out_draws) {
    if (!mem || !fifo_size || fifo_base + 4 > mem_size) {
        if (out_methods) *out_methods = 0;
        if (out_draws) *out_draws = 0;
        return get;
    }
    put %= fifo_size;
    get %= fifo_size;
    uint32_t methods = 0, draws = 0;
    uint32_t guard = 0;
    uint32_t ret_stack = 0;
    int has_ret = 0;
    while (get != put && guard++ < 8192u) {
        uint32_t hdr = rd_fifo_u32(mem, mem_size, fifo_base, get, fifo_size);
        /* JUMP */
        if ((hdr & 0xe0000003u) == 0x20000000u) {
            get = (hdr & 0x1ffffffcu) % fifo_size;
            methods++;
            continue;
        }
        /* CALL */
        if ((hdr & 0x3u) == 0x1u) {
            ret_stack = (get + 4) % fifo_size;
            has_ret = 1;
            get = (hdr & 0x1ffffffcu) % fifo_size;
            methods++;
            continue;
        }
        /* RETURN */
        if ((hdr & 0x3u) == 0x2u || (hdr & 0xe0000003u) == 0x00020000u) {
            if (has_ret) { get = ret_stack; has_ret = 0; }
            else get = (get + 4) % fifo_size;
            methods++;
            continue;
        }
        get = (get + 4) % fifo_size;
        uint32_t method = 0, count = 1;
        rsx_fifo_decode_header(hdr, &method, &count);
        if (count > 512u) count = 512u;
        for (uint32_t i = 0; i < count && get != put; i++) {
            (void)rd_fifo_u32(mem, mem_size, fifo_base, get, fifo_size);
            get = (get + 4) % fifo_size;
            rsx_method_kind k = rsx_classify_method(method + i * 4u);
            methods++;
            if (rsx_method_is_drawish(k)) draws++;
        }
    }
    if (out_methods) *out_methods = methods;
    if (out_draws) *out_draws = draws;
    return get;
}

int rsx_fifo_drain_publish(uint8_t* mem, uint64_t mem_size,
                           uint64_t ctrl, uint64_t fifo_base, uint32_t fifo_size,
                           uint32_t* out_methods, uint32_t* out_draws) {
    if (!mem || !ctrl || ctrl + 12 > mem_size) return 0;
    if (!fifo_size) fifo_size = 0x100000u;
    uint32_t put = be32(mem + ctrl + 0) % fifo_size;
    uint32_t get = be32(mem + ctrl + 8) % fifo_size;
    uint32_t ref = be32(mem + ctrl + 4) % fifo_size;
    if (get == put && ref != put) get = ref;
    if (put == get) {
        if (out_methods) *out_methods = 0;
        if (out_draws) *out_draws = 0;
        return 0;
    }
    uint32_t methods = 0, draws = 0;
    uint32_t nget = get;
    if (fifo_base && fifo_base + fifo_size <= mem_size)
        nget = rsx_fifo_drain_walk(mem, mem_size, fifo_base, fifo_size, put, get, &methods, &draws);
    else {
        /* No base: coarse step toward PUT */
        rsx_fifo_drain_control(mem, mem_size, ctrl, fifo_size);
        nget = be32(mem + ctrl + 8) % fifo_size;
        methods = 1;
    }
    wbe32(mem + ctrl + 8, nget);
    wbe32(mem + ctrl + 4, put);
    if (out_methods) *out_methods = methods;
    if (out_draws) *out_draws = draws;
    return methods > 0 || nget != get;
}
