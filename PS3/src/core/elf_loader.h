// elf_loader.h - Phase 2: parse a decrypted PS3 ELF (ELF64, big-endian, EM_PPC64).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ps3 {

struct Segment {
    uint32_t type = 0;      // p_type (1 = PT_LOAD)
    uint32_t flags = 0;     // p_flags (1 = X, 2 = W, 4 = R)
    uint64_t vaddr = 0;
    uint64_t filesz = 0;
    uint64_t memsz = 0;
    std::vector<uint8_t> data;   // filesz bytes
    bool executable() const { return (flags & 1u) != 0; }
};

struct ElfImage {
    uint16_t machine = 0;       // 21 = PowerPC64
    uint16_t type = 0;          // 2 = EXEC, 0xFFA4 = PS3 PRX
    uint64_t entry = 0;         // PS3: address of a function descriptor (OPD)
    std::vector<Segment> segments;
    uint64_t fingerprint = 0;

    // Read a big-endian value from guest memory described by PT_LOAD segments.
    bool read64(uint64_t addr, uint64_t& out) const;
    bool read32(uint64_t addr, uint32_t& out) const;
};

// Returns true on success; otherwise fills 'err'.
bool load_elf(const std::string& path, ElfImage& out, std::string& err);

} // namespace ps3
