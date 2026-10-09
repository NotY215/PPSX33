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

// PowerPC64 function descriptor (OPD entry): code entry + TOC + env
struct OpdEntry {
    uint64_t addr = 0;   // address of the descriptor itself
    uint64_t entry = 0;  // code VA
    uint64_t toc = 0;
};

struct ElfSymbol {
    std::string name;
    uint64_t value = 0;
    uint64_t size = 0;
    uint8_t info = 0;
    uint16_t shndx = 0;
    bool is_func() const { return (info & 0x0F) == 2; }
};

// Minimal SCE / PRX import stub record (name + tentative NID if present)
struct PrxImport {
    std::string module;
    std::string name;
    uint32_t nid = 0;
    uint64_t stub_addr = 0;
};

// Embedded SPU image found inside PPU segments
struct SpuImage {
    uint64_t host_addr = 0; // VA in PPU space where the image was found
    std::vector<uint8_t> data;
};

struct ElfImage {
    uint16_t machine = 0;       // 21 = PowerPC64
    uint16_t type = 0;          // 2 = EXEC, 0xFFA4 = PS3 PRX
    uint64_t entry = 0;         // PS3: address of a function descriptor (OPD)
    std::vector<Segment> segments;
    std::vector<OpdEntry> opds;
    std::vector<ElfSymbol> symbols;
    std::vector<PrxImport> prx_imports;
    std::vector<SpuImage> spu_images;
    uint64_t fingerprint = 0;

    bool read64(uint64_t addr, uint64_t& out) const;
    bool read32(uint64_t addr, uint32_t& out) const;
    bool read_bytes(uint64_t addr, void* dst, size_t n) const;
};

// Returns true on success; otherwise fills 'err'.
bool load_elf(const std::string& path, ElfImage& out, std::string& err);

// After load_elf: discover OPDs, symbols, sceStub strings, embedded SPU ELFs.
void analyze_elf(ElfImage& img);

} // namespace ps3
