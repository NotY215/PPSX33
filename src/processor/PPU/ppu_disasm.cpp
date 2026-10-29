// WORK: Minimal disasm helper.
#include "ppu_disasm.h"
#include <cstdio>
namespace ppsx33::ppu {
std::string disasm_one(uint32_t w,uint64_t pc){char b[64];std::snprintf(b,sizeof b,"0x%08X @ %llx",w,(unsigned long long)pc);return b;}
}
