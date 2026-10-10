#include "elf_loader.h"
#include "ps3_util.h"
#include <fstream>
#include <cstring>

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
    uint64_t shoff = ps3_be64(&b[40]);
    uint16_t phentsize = ps3_be16(&b[54]);
    uint16_t phnum = ps3_be16(&b[56]);
    uint16_t shentsize = ps3_be16(&b[58]);
    uint16_t shnum = ps3_be16(&b[60]);
    uint16_t shstrndx = ps3_be16(&b[62]);
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
        if (s.type != 1) continue;
        if (off + s.filesz > b.size()) { err = "Segment exceeds file size."; return false; }
        s.data.assign(b.begin() + (size_t)off, b.begin() + (size_t)(off + s.filesz));
        out.segments.push_back(std::move(s));
    }
    if (out.segments.empty()) { err = "ELF has no PT_LOAD segments."; return false; }
    out.fingerprint = ps3_fnv1a64(b.data(), b.size());

    if (shentsize >= 64 && shnum > 0 && shoff + (uint64_t)shnum * shentsize <= b.size()) {
        auto sh_at = [&](uint16_t i) { return &b[shoff + (uint64_t)i * shentsize]; };
        const uint8_t* shstr = nullptr;
        uint64_t shstr_size = 0;
        if (shstrndx < shnum) {
            uint64_t shstr_off = ps3_be64(sh_at(shstrndx) + 24);
            shstr_size = ps3_be64(sh_at(shstrndx) + 32);
            if (shstr_off + shstr_size <= b.size()) shstr = &b[shstr_off];
        }
        int sym_idx = -1, str_idx = -1;
        for (uint16_t i = 0; i < shnum; ++i) {
            const uint8_t* sh = sh_at(i);
            uint32_t name_off = ps3_be32(sh + 0);
            uint32_t type = ps3_be32(sh + 4);
            std::string nm;
            if (shstr && name_off < shstr_size) nm = (const char*)(shstr + name_off);
            if (type == 2 || nm == ".symtab") sym_idx = i;
            if (type == 3 && nm == ".strtab") str_idx = i;
        }
        if (sym_idx >= 0) {
            const uint8_t* sh = sh_at((uint16_t)sym_idx);
            uint64_t soff = ps3_be64(sh + 24);
            uint64_t ssize = ps3_be64(sh + 32);
            uint64_t entsz = ps3_be64(sh + 56);
            if (entsz < 24) entsz = 24;
            const uint8_t* strtab = nullptr;
            uint64_t str_size = 0;
            if (str_idx >= 0) {
                const uint8_t* sth = sh_at((uint16_t)str_idx);
                uint64_t stoff = ps3_be64(sth + 24);
                str_size = ps3_be64(sth + 32);
                if (stoff + str_size <= b.size()) strtab = &b[stoff];
            }
            if (soff + ssize <= b.size()) {
                for (uint64_t o = 0; o + entsz <= ssize; o += entsz) {
                    const uint8_t* sym = &b[soff + o];
                    ElfSymbol s;
                    uint32_t nidx = ps3_be32(sym + 0);
                    s.info = sym[4];
                    s.shndx = ps3_be16(sym + 6);
                    s.value = ps3_be64(sym + 8);
                    s.size = ps3_be64(sym + 16);
                    if (strtab && nidx < str_size) s.name = (const char*)(strtab + nidx);
                    if (!s.name.empty() || s.value) out.symbols.push_back(std::move(s));
                }
            }
        }
    }

    analyze_elf(out);
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
bool ElfImage::read_bytes(uint64_t addr, void* dst, size_t n) const {
    for (const auto& s : segments)
        if (addr >= s.vaddr && addr + n <= s.vaddr + s.filesz) {
            std::memcpy(dst, &s.data[(size_t)(addr - s.vaddr)], n);
            return true;
        }
    return false;
}

static bool is_exec_addr(const ElfImage& img, uint64_t a) {
    for (const auto& s : img.segments)
        if (s.executable() && a >= s.vaddr && a < s.vaddr + s.filesz) return true;
    return false;
}

// GOW3 OPD often appears as (entry_pc << 32) | toc when mis-read as one 64-bit;
// normalize to entry=low/high split matching runtime (pc=0x10230 toc=0x52d6c8).
static void normalize_opd(const ElfImage& img, OpdEntry& e) {
    if (e.entry > 0xFFFFFFFFull) {
        uint32_t hi = (uint32_t)(e.entry >> 32);
        uint32_t lo = (uint32_t)e.entry;
        if (hi != 0 && hi < 0x01000000u && lo < 0x10000000u) {
            if (is_exec_addr(img, hi) || !is_exec_addr(img, e.entry)) {
                e.entry = hi;
                if (e.toc == 0 || e.toc > 0xFFFFFFFFull) e.toc = lo;
            }
        }
    }
    // Prefer low 32 bits if full value is not in an executable segment
    if (!is_exec_addr(img, e.entry)) {
        uint64_t low = e.entry & 0xFFFFFFFFull;
        if (low && is_exec_addr(img, low)) e.entry = low;
    }
    if (e.toc > 0xFFFFFFFFull)
        e.toc &= 0xFFFFFFFFull;
}

void analyze_elf(ElfImage& img) {
    img.opds.clear();
    img.prx_imports.clear();
    img.spu_images.clear();

    {
        OpdEntry e;
        e.addr = img.entry;
        uint64_t a = 0, t = 0;
        // Prefer 64-bit; also try 32-bit pair (some descriptors pack tightly in practice)
        if (img.read64(img.entry, a) && img.read64(img.entry + 8, t)) {
            e.entry = a; e.toc = t;
            normalize_opd(img, e);
            img.opds.push_back(e);
        } else {
            uint32_t a32 = 0, t32 = 0;
            if (img.read32(img.entry, a32) && img.read32(img.entry + 4, t32)) {
                e.entry = a32; e.toc = t32;
                normalize_opd(img, e);
                img.opds.push_back(e);
            }
        }
    }

    for (const auto& s : img.segments) {
        if (s.executable()) continue;
        if (s.data.size() < 16) continue;
        for (size_t off = 0; off + 16 <= s.data.size(); off += 8) {
            uint64_t code = ps3_be64(&s.data[off]);
            uint64_t toc  = ps3_be64(&s.data[off + 8]);
            OpdEntry e;
            e.addr = s.vaddr + off;
            e.entry = code;
            e.toc = toc;
            normalize_opd(img, e);
            if (!is_exec_addr(img, e.entry)) continue;
            if (e.toc < 0x10000 || e.toc > 0xFFFFFFFFull) continue;
            img.opds.push_back(e);
            if (img.opds.size() > 4096) break;
        }
        if (img.opds.size() > 4096) break;
    }

    for (const auto& s : img.segments) {
        const auto& d = s.data;
        for (size_t i = 0; i + 8 < d.size(); ++i) {
            if (!((d[i] >= 'a' && d[i] <= 'z') || (d[i] >= 'A' && d[i] <= 'Z'))) continue;
            size_t j = i;
            while (j < d.size() && d[j] != 0 && j - i < 64) ++j;
            if (j >= d.size() || d[j] != 0) continue;
            std::string name((const char*)&d[i], j - i);
            bool interesting =
                name.find("cell") != std::string::npos ||
                name.rfind("sys_", 0) == 0 ||
                name.find("sce") != std::string::npos ||
                name.rfind("lib", 0) == 0;
            if (!interesting || name.size() < 4) { i = j; continue; }
            // Skip long format/error strings
            if (name.find(' ') != std::string::npos && name.size() > 24) { i = j; continue; }
            PrxImport im;
            im.name = name;
            im.stub_addr = s.vaddr + i;
            if (i >= 4) im.nid = ps3_be32(&d[i - 4]);
            img.prx_imports.push_back(std::move(im));
            if (img.prx_imports.size() > 2048) break;
            i = j;
        }
        if (img.prx_imports.size() > 2048) break;
    }

    for (const auto& s : img.segments) {
        const auto& d = s.data;
        for (size_t i = 0; i + 64 < d.size(); ++i) {
            if (d[i] != 0x7F || d[i+1] != 'E' || d[i+2] != 'L' || d[i+3] != 'F') continue;
            if (d[i+4] != 1) continue;
            uint16_t mach = (uint16_t)((d[i+18] << 8) | d[i+19]);
            if (mach != 23 && mach != 0x17) continue;
            size_t max_len = d.size() - i;
            if (max_len > 512 * 1024) max_len = 512 * 1024;
            SpuImage sp;
            sp.host_addr = s.vaddr + i;
            sp.data.assign(d.begin() + (std::ptrdiff_t)i, d.begin() + (std::ptrdiff_t)(i + max_len));
            while (sp.data.size() > 256 && sp.data.back() == 0) sp.data.pop_back();
            img.spu_images.push_back(std::move(sp));
            if (img.spu_images.size() > 32) break;
            i += 256;
        }
        if (img.spu_images.size() > 32) break;
    }
}

} // namespace ps3
