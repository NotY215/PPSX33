#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

uint32_t ps3rt_be32_load(const void* p);
uint64_t ps3rt_be64_load(const void* p);
void     ps3rt_be32_store(void* p, uint32_t v);
void     ps3rt_be64_store(void* p, uint64_t v);
void     ps3rt_memcpy_bswap32(void* dst, const void* src, size_t bytes);

#ifdef __cplusplus
}
#endif
