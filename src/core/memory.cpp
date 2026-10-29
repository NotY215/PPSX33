// WORK: Guest address space helpers.
#include "memory.h"
namespace ppsx33::core {
static uint8_t* g_base=nullptr; static size_t g_sz=0;
bool memory_map(uint8_t* b,size_t s){g_base=b;g_sz=s;return b!=nullptr;}
uint8_t* memory_base(){return g_base;}
size_t memory_size(){return g_sz;}
}
