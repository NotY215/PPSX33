#include "ps3_util.h"

uint16_t ps3_bswap16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }
uint32_t ps3_bswap32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
           ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
}
uint64_t ps3_bswap64(uint64_t v) {
    return ((uint64_t)ps3_bswap32((uint32_t)v) << 32) | ps3_bswap32((uint32_t)(v >> 32));
}
uint16_t ps3_be16(const uint8_t* p) { return (uint16_t)((p[0] << 8) | p[1]); }
uint32_t ps3_be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
uint64_t ps3_be64(const uint8_t* p) {
    return ((uint64_t)ps3_be32(p) << 32) | ps3_be32(p + 4);
}
uint64_t ps3_fnv1a64(const uint8_t* data, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    size_t i;
    for (i = 0; i < len; ++i) { h ^= data[i]; h *= 1099511628211ULL; }
    return h;
}
