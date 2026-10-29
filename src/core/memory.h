#pragma once
// WORK: Guest main RAM arena mapping.
#include <cstdint>
#include <cstddef>
namespace ppsx33::core {
bool memory_map(uint8_t* base, size_t size);
uint8_t* memory_base();
size_t memory_size();
}
