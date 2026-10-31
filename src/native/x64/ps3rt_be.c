/* Fast big-endian helpers for guest memory (translation + RSX FIFO). */
#include "ps3rt_be.h"
#include <string.h>

#if defined(_MSC_VER)
#include <stdlib.h>
#define BSWAP32(x) _byteswap_ulong(x)
#define BSWAP64(x) _byteswap_uint64(x)
#else
#define BSWAP32(x) __builtin_bswap32(x)
#define BSWAP64(x) __builtin_bswap64(x)
#endif

uint32_t ps3rt_be32_load(const void* p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return BSWAP32(v);
}

uint64_t ps3rt_be64_load(const void* p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return BSWAP64(v);
}

void ps3rt_be32_store(void* p, uint32_t v) {
    uint32_t o = BSWAP32(v);
    memcpy(p, &o, 4);
}

void ps3rt_be64_store(void* p, uint64_t v) {
    uint64_t o = BSWAP64(v);
    memcpy(p, &o, 8);
}

void ps3rt_memcpy_bswap32(void* dst, const void* src, size_t bytes) {
    const uint8_t* s = (const uint8_t*)src;
    uint8_t* d = (uint8_t*)dst;
    size_t n = bytes & ~3u;
    size_t i;
    for (i = 0; i < n; i += 4) {
        uint32_t v;
        memcpy(&v, s + i, 4);
        v = BSWAP32(v);
        memcpy(d + i, &v, 4);
    }
    for (; i < bytes; ++i) d[i] = s[i];
}
