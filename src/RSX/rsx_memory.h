#pragma once
#include <cstdint>
namespace ppsx33 { namespace rsx {
void rsx_memory_init();
void rsx_memory_set_local(uint64_t ea, uint32_t size);
void rsx_memory_set_main(uint64_t ea, uint32_t size);
uint64_t rsx_memory_local_ea();
uint32_t rsx_memory_local_size();
}}
