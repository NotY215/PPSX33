#include "elf_loader.h"
#include "ps3_util.h"
#include <fstream>

namespace ps3 {

static bool read_all(const std::string& path, std::vector<uint8_t>& buf) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamoff n = f.tellg();
    f.seekg(0);
    if (n <= 0) return false;
    buf.resize((size_t)n);
    f.read((char*)buf.data(), n);
    return (bool)f;
}

bool load_elf(const std::string& path, ElfImage& out, std::string& err) {
    std::vector<uint8_t> b;
    if (!read_all(path, b)) { err = "Cannot read file: " + path; return false; }
    if (b.size() < 64 || b[0] != 0x7F || b[1] != 'E' || b[2] != 'L' || b[3] != 'F') {
        err = "Not an ELF file (is EBOOT.BIN still encrypted? Decrypt it first via RPCS3)."; return false;
    }
    if (b[4] != 2 || b[5] != 2) { err = "Expected ELF64 big-endian (PS3 PPU)."; return false; }

    out = ElfImage();
    out.type    = ps3_be16(&b[16]);
    out.machine = ps3_be16(&b[18]);
    out.entry   = ps3_be64(&b[24]);
    uint64_t phoff = ps3_be64(&b[32]);
    uint16_t phentsize = ps3_be16(&b[54]);
    uint16_t phnum = ps3_be16(&b[56]);
    if (out.machine != 21) { err = "Not a PowerPC64 ELF (e_machine != 21)."; return false; }
    if (phentsize < 56 || phoff + (uint64_t)phnum * phentsize > b.size()) {
        err = "Corrupt program header table."; return false;
    }
    for (uint16_t i = 0; i < phnum; ++i) {
        const uint8_t* ph = &b[phoff + (uint64_t)i * phentsize];
        Segment s;
        s.type  = ps3_be32(ph + 0);
        s.flags = ps3_be32(ph + 4);
        uint64_t off = ps3_be64(ph + 8);
        s.vaddr  = ps3_be64(ph + 16);
        s.filesz = ps3_be64(ph + 32);
        s.memsz  = ps3_be64(ph + 40);
        if (s.type != 1) continue;  // PT_LOAD only
        if (off + s.filesz > b.size()) { err = "Segment exceeds file size."; return false; }
        s.data.assign(b.begin() + (size_t)off, b.begin() + (size_t)(off + s.filesz));
        out.segments.push_back(std::move(s));
    }
    if (out.segments.empty()) { err = "ELF has no PT_LOAD segments."; return false; }
    out.fingerprint = ps3_fnv1a64(b.data(), b.size());
    return true;
}

bool ElfImage::read32(uint64_t addr, uint32_t& v) const {
    for (const auto& s : segments)
        if (addr >= s.vaddr && addr + 4 <= s.vaddr + s.filesz) {
            v = ps3_be32(&s.data[(size_t)(addr - s.vaddr)]); return true;
        }
    return false;
}
bool ElfImage::read64(uint64_t addr, uint64_t& v) const {
    for (const auto& s : segments)
        if (addr >= s.vaddr && addr + 8 <= s.vaddr + s.filesz) {
            v = ps3_be64(&s.data[(size_t)(addr - s.vaddr)]); return true;
        }
    return false;
}

} // namespace ps3
