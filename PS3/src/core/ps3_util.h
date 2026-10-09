/* ps3_util.h - plain C helpers shared by the C++ core (language: C).
 * PS3 binaries are big-endian; the host (x86-64) is little-endian. */
#ifndef PS3_UTIL_H
#define PS3_UTIL_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

uint16_t ps3_bswap16(uint16_t v);
uint32_t ps3_bswap32(uint32_t v);
uint64_t ps3_bswap64(uint64_t v);

/* Read big-endian values from a raw byte buffer. */
uint16_t ps3_be16(const uint8_t* p);
uint32_t ps3_be32(const uint8_t* p);
uint64_t ps3_be64(const uint8_t* p);

/* FNV-1a 64 bit hash (used to fingerprint an ELF so caches can be reused). */
uint64_t ps3_fnv1a64(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif
#endif
