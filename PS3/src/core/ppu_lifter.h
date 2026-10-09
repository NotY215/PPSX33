// ppu_lifter.h - Phase 2/3: PowerPC (PPU) -> C++ source generator ("static recompiler").
#pragma once
#include "elf_loader.h"
#include <map>
#include <string>

namespace ps3 {

struct LiftStats {
    uint64_t instructions = 0;
    uint64_t implemented = 0;
    uint64_t unimplemented = 0;
    uint32_t chunks = 0;
    std::map<std::string, uint64_t> missing;   // "op=31 xo=215" -> count
};

// Writes ppu_chunk_NNN.cpp, ppu_chunks.cpp, image_info.h, game_main.cpp,
// guest_image.bin and lift_report.txt into codebase_dir.
bool lift_elf(const ElfImage& elf, const std::string& codebase_dir,
              LiftStats& stats, std::string& err);

} // namespace ps3
