#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void     ps3rt_import_bind(uint64_t ea, uint32_t nid);
uint32_t ps3rt_import_lookup(uint64_t ea);
void     ps3rt_import_clear(void);
int      ps3rt_import_bind_count(void);

#ifdef __cplusplus
}
#endif
