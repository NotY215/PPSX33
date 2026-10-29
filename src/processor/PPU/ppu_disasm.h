#pragma once
// WORK: PPU disassembly for diagnostics.
#include <string>
#include <cstdint>
namespace ppsx33::ppu { std::string disasm_one(uint32_t be_word, uint64_t pc); }
