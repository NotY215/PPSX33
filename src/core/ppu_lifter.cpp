#include "ppu_lifter.h"
#include "ps3_util.h"
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace ps3 {
namespace {

constexpr uint32_t kChunkInsns = 8192;   // instructions per generated function

std::string fmt(const char* f, ...) {
    char buf[512];
    va_list ap; va_start(ap, f);
    vsnprintf(buf, sizeof buf, f, ap);
    va_end(ap);
    return buf;
}

// PowerPC mask: bits MB..ME inclusive, bit 0 = MSB (64-bit numbering).
uint64_t mask64(unsigned mb, unsigned me) {
    uint64_t a = ~0ULL >> mb, b = ~0ULL << (63 - me);
    return mb <= me ? (a & b) : ~(( ~0ULL >> (me + 1)) & (~0ULL << (63 - (mb - 1))));
}

struct Emit {
    uint64_t pc, cs, ce;   // current pc, chunk start/end
    std::ostringstream o;

    std::string jump_const(uint64_t t) const {
        if (t >= cs && t < ce) return fmt("pc = 0x%llxull; goto dispatch;", (unsigned long long)t);
        return fmt("c.pc = 0x%llxull; return true;", (unsigned long long)t);
    }
    std::string jump_dyn(const char* reg) const {
        return fmt("pc = %s & ~3ull; if (pc >= 0x%llxull && pc < 0x%llxull) goto dispatch; c.pc = pc; return true;",
                   reg, (unsigned long long)cs, (unsigned long long)ce);
    }
};

// Returns true if instruction was translated.
bool lift_one(uint32_t w, Emit& e, std::string& key) {
    const unsigned op = w >> 26, rt = (w >> 21) & 31, ra = (w >> 16) & 31, rb = (w >> 11) & 31;
    const int64_t simm = (int16_t)w;
    const uint64_t uimm = w & 0xFFFF;
    const bool rc = w & 1;
    auto& o = e.o;
    const unsigned long long next = e.pc + 4;
    auto ea = [&](int64_t d) {   // (RA|0) + d
        return ra ? fmt("(c.gpr[%u] + (int64_t)%lld)", ra, (long long)d) : fmt("(uint64_t)%lld", (long long)d);
    };
    auto rc0 = [&](unsigned r) { if (rc) o << fmt(" set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", r); };

    switch (op) {
    case 14: // addi / li
        o << (ra ? fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)simm)
                 : fmt("c.gpr[%u] = (uint64_t)%lld;", rt, (long long)simm)); return true;
    case 15: // addis / lis
        o << (ra ? fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)(simm << 16))
                 : fmt("c.gpr[%u] = (uint64_t)%lld;", rt, (long long)(simm << 16))); return true;
    case 24: o << fmt("c.gpr[%u] = c.gpr[%u] | 0x%llxull;", ra, rt, (unsigned long long)uimm); return true;          // ori
    case 25: o << fmt("c.gpr[%u] = c.gpr[%u] | 0x%llxull;", ra, rt, (unsigned long long)(uimm << 16)); return true;   // oris
    case 26: o << fmt("c.gpr[%u] = c.gpr[%u] ^ 0x%llxull;", ra, rt, (unsigned long long)uimm); return true;          // xori
    case 27: o << fmt("c.gpr[%u] = c.gpr[%u] ^ 0x%llxull;", ra, rt, (unsigned long long)(uimm << 16)); return true;   // xoris
    case 28: o << fmt("c.gpr[%u] = c.gpr[%u] & 0x%llxull; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", ra, rt, (unsigned long long)uimm, ra); return true; // andi.
    case 29: o << fmt("c.gpr[%u] = c.gpr[%u] & 0x%llxull; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", ra, rt, (unsigned long long)(uimm << 16), ra); return true; // andis.
    case 32: o << fmt("c.gpr[%u] = rd32(c, %s);", rt, ea(simm).c_str()); return true;   // lwz
    case 33: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd32(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // lwzu
    case 34: o << fmt("c.gpr[%u] = rd8(c, %s);", rt, ea(simm).c_str()); return true;    // lbz
    case 40: o << fmt("c.gpr[%u] = rd16(c, %s);", rt, ea(simm).c_str()); return true;   // lhz
    case 36: o << fmt("wr32(c, %s, (uint32_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true;   // stw
    case 37: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr32(c, a, (uint32_t)c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // stwu
    case 38: o << fmt("wr8(c, %s, (uint8_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true;     // stb
    case 44: o << fmt("wr16(c, %s, (uint16_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true;   // sth
    case 58: { // DS-form loads
        int64_t ds = (int16_t)(w & 0xFFFC);
        unsigned x = w & 3;
        if (x == 0) { o << fmt("c.gpr[%u] = rd64(c, %s);", rt, ea(ds).c_str()); return true; }                       // ld
        if (x == 1) { o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd64(c, a); c.gpr[%u] = a; }", ra, (long long)ds, rt, ra); return true; } // ldu
        if (x == 2) { o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int32_t)rd32(c, %s);", rt, ea(ds).c_str()); return true; } // lwa
        break; }
    case 62: { // DS-form stores
        int64_t ds = (int16_t)(w & 0xFFFC);
        unsigned x = w & 3;
        if (x == 0) { o << fmt("wr64(c, %s, c.gpr[%u]);", ea(ds).c_str(), rt); return true; }                          // std
        if (x == 1) { o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr64(c, a, c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)ds, rt, ra); return true; } // stdu
        break; }
    case 11: { // cmpi
        unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
        o << (l ? fmt("set_cr_signed(c, %u, (int64_t)c.gpr[%u], %lld);", bf, ra, (long long)simm)
                : fmt("set_cr_signed(c, %u, (int32_t)c.gpr[%u], %lld);", bf, ra, (long long)simm)); return true; }
    case 10: { // cmpli
        unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
        o << (l ? fmt("set_cr_unsigned(c, %u, c.gpr[%u], 0x%llxull);", bf, ra, (unsigned long long)uimm)
                : fmt("set_cr_unsigned(c, %u, (uint32_t)c.gpr[%u], 0x%llxull);", bf, ra, (unsigned long long)uimm)); return true; }
    case 21: { // rlwinm
        unsigned sh = rb, mb = (w >> 6) & 31, me = (w >> 1) & 31;
        uint64_t m = mask64(mb + 32, me + 32);
        o << fmt("{ uint64_t v = (uint32_t)c.gpr[%u]; v = (v << 32) | v; v = (v << %u) | (%u ? (v >> (64 - %u)) : 0); c.gpr[%u] = v & 0x%llxull; }",
                 rt, sh, sh, sh, ra, (unsigned long long)m);
        rc0(ra); return true; }
    case 18: { // b, ba, bl, bla
        int64_t li = ((int32_t)(w & 0x03FFFFFC) << 6) >> 6;
        uint64_t t = (w & 2) ? (uint64_t)li : e.pc + (uint64_t)li;
        if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
        o << e.jump_const(t); return true; }
    case 16: { // bc
        unsigned bo = rt, bi = ra;
        int64_t bd = (int16_t)(w & 0xFFFC);
        uint64_t t = (w & 2) ? (uint64_t)bd : e.pc + (uint64_t)bd;
        o << fmt("if (bc_taken(c, %u, %u)) { ", bo, bi);
        if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
        o << e.jump_const(t) << " }"; return true; }
    case 17: o << fmt("ps3rt_syscall(&c); c.pc = 0x%llxull; return true;", next); return true; // sc
    case 19: {
        unsigned xo = (w >> 1) & 0x3FF;
        if (xo == 16 || xo == 528) { // bclr / bcctr
            const char* tgt = (xo == 16) ? "c.lr" : "c.ctr";
            o << fmt("{ uint64_t t = %s; ", tgt);
            if ((rt & 0x14) == 0x14) { // unconditional
                if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
                o << fmt("pc = t & ~3ull; if (pc >= 0x%llxull && pc < 0x%llxull) goto dispatch; c.pc = pc; return true; }",
                         (unsigned long long)e.cs, (unsigned long long)e.ce);
            } else {
                o << fmt("if (bc_taken(c, %u, %u)) { ", rt, ra);
                if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
                o << fmt("pc = t & ~3ull; if (pc >= 0x%llxull && pc < 0x%llxull) goto dispatch; c.pc = pc; return true; } }",
                         (unsigned long long)e.cs, (unsigned long long)e.ce);
            }
            return true;
        }
        key = fmt("op=19 xo=%u", xo); return false; }
    case 31: {
        unsigned xo = (w >> 1) & 0x3FF;
        switch (xo) {
        case 266: o << fmt("c.gpr[%u] = c.gpr[%u] + c.gpr[%u];", rt, ra, rb); rc0(rt); return true;          // add
        case 40:  o << fmt("c.gpr[%u] = c.gpr[%u] - c.gpr[%u];", rt, rb, ra); rc0(rt); return true;          // subf
        case 444: o << fmt("c.gpr[%u] = c.gpr[%u] | c.gpr[%u];", ra, rt, rb); rc0(ra); return true;          // or / mr
        case 28:  o << fmt("c.gpr[%u] = c.gpr[%u] & c.gpr[%u];", ra, rt, rb); rc0(ra); return true;          // and
        case 316: o << fmt("c.gpr[%u] = c.gpr[%u] ^ c.gpr[%u];", ra, rt, rb); rc0(ra); return true;          // xor
        case 0: case 32: { // cmp / cmpl
            unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
            if (xo == 0) o << (l ? fmt("set_cr_signed(c, %u, (int64_t)c.gpr[%u], (int64_t)c.gpr[%u]);", bf, ra, rb)
                                 : fmt("set_cr_signed(c, %u, (int32_t)c.gpr[%u], (int32_t)c.gpr[%u]);", bf, ra, rb));
            else         o << (l ? fmt("set_cr_unsigned(c, %u, c.gpr[%u], c.gpr[%u]);", bf, ra, rb)
                                 : fmt("set_cr_unsigned(c, %u, (uint32_t)c.gpr[%u], (uint32_t)c.gpr[%u]);", bf, ra, rb));
            return true; }
        case 23:  o << fmt("c.gpr[%u] = rd32(c, %s + c.gpr[%u]);", rt, ra ? fmt("c.gpr[%u]", ra).c_str() : "0ull", rb); return true;           // lwzx
        case 151: o << fmt("wr32(c, %s + c.gpr[%u], (uint32_t)c.gpr[%u]);", ra ? fmt("c.gpr[%u]", ra).c_str() : "0ull", rb, rt); return true;   // stwx
        case 339: case 467: { // mfspr / mtspr  (spr field halves are swapped)
            unsigned spr = ((w >> 16) & 31) | (((w >> 11) & 31) << 5);
            const char* r = spr == 8 ? "c.lr" : spr == 9 ? "c.ctr" : spr == 1 ? "c.xer" : nullptr;
            if (!r) { key = fmt("op=31 xo=%u spr=%u", xo, spr); return false; }
            o << (xo == 339 ? fmt("c.gpr[%u] = %s;", rt, r) : fmt("%s = c.gpr[%u];", r, rt)); return true; }
        default: key = fmt("op=31 xo=%u", xo); return false;
        }
    }
    default: break;
    }
    if (key.empty()) key = fmt("op=%u", op);
    return false;
}

bool write_file(const fs::path& p, const std::string& s) {
    std::ofstream f(p, std::ios::binary);
    if (!f) return false;
    f.write(s.data(), (std::streamsize)s.size());
    return (bool)f;
}

} // namespace

bool lift_elf(const ElfImage& elf, const std::string& dir, LiftStats& st, std::string& err) {
    fs::create_directories(dir);
    std::ostringstream table;   // ppu_chunks.cpp body
    std::ostringstream decls;
    st = LiftStats();

    for (const auto& seg : elf.segments) {
        if (!seg.executable()) continue;
        const uint64_t nInsn = seg.filesz / 4;
        for (uint64_t base = 0; base < nInsn; base += kChunkInsns) {
            uint64_t cnt = std::min<uint64_t>(kChunkInsns, nInsn - base);
            Emit e;
            e.cs = seg.vaddr + base * 4;
            e.ce = e.cs + cnt * 4;
            std::ostringstream f;
            f << "// GENERATED by ps3core (Phase 2/3). Do not edit.\n#include \"ppu_runtime.h\"\n";
            f << "bool ppu_chunk_" << st.chunks << "(PPUContext& c) {\n    uint64_t pc = c.pc;\ndispatch:\n    switch (pc) {\n";
            for (uint64_t i = 0; i < cnt; ++i) {
                e.pc = e.cs + i * 4;
                e.o.str(""); e.o.clear();
                uint32_t w = ps3_be32(&seg.data[(size_t)((base + i) * 4)]);
                std::string key;
                ++st.instructions;
                f << fmt("    case 0x%llxull: // %08x\n        ", (unsigned long long)e.pc, w);
                if (lift_one(w, e, key)) { ++st.implemented; f << e.o.str() << "\n"; }
                else {
                    ++st.unimplemented; ++st.missing[key];
                    f << fmt("ps3rt_unimplemented(&c, 0x%08x, 0x%llxull); c.halted = true; c.pc = 0x%llxull; return true;\n",
                             w, (unsigned long long)e.pc, (unsigned long long)e.pc);
                }
            }
            f << "    default: c.pc = pc; c.halted = true; return true;   // in range but not an instruction boundary\n    }\n";
            f << fmt("    c.pc = 0x%llxull; return true;\n}\n", (unsigned long long)e.ce);
            char name[64]; snprintf(name, sizeof name, "ppu_chunk_%03u.cpp", st.chunks);
            if (!write_file(fs::path(dir) / name, f.str())) { err = "Cannot write " + std::string(name); return false; }
            decls << "bool ppu_chunk_" << st.chunks << "(PPUContext& c);\n";
            table << fmt("    { 0x%llxull, 0x%llxull, ppu_chunk_%u },\n", (unsigned long long)e.cs, (unsigned long long)e.ce, st.chunks);
            ++st.chunks;
        }
    }
    if (st.chunks == 0) { err = "No executable segments found."; return false; }

    std::ostringstream t;
    t << "// GENERATED chunk table\n#include \"ppu_runtime.h\"\n" << decls.str()
      << "extern const PPUChunk g_ppu_chunks[] = {\n" << table.str() << "};\n"
      << "extern const size_t g_ppu_chunk_count = " << st.chunks << ";\n";
    if (!write_file(fs::path(dir) / "ppu_chunks.cpp", t.str())) { err = "Cannot write ppu_chunks.cpp"; return false; }

    // Entry point: PS3 e_entry points at an OPD {code address, TOC}.
    uint64_t pc = 0, toc = 0;
    if (!elf.read64(elf.entry, pc) || !elf.read64(elf.entry + 8, toc)) { pc = elf.entry; toc = 0; }
    std::ostringstream info;
    info << "// GENERATED\n#pragma once\n#include <cstdint>\n"
         << fmt("static const uint64_t kEntryPc  = 0x%llxull;\nstatic const uint64_t kEntryToc = 0x%llxull;\n",
                (unsigned long long)pc, (unsigned long long)toc);
    if (!write_file(fs::path(dir) / "image_info.h", info.str())) { err = "Cannot write image_info.h"; return false; }

    static const char* mainSrc =
        "// GENERATED game entry point (template lives in src/core/ppu_lifter.cpp)\n"
        "#include \"ppu_runtime.h\"\n#include \"image_info.h\"\n#include <cstdio>\n#include <cstring>\n#include <string>\n"
        "extern const PPUChunk g_ppu_chunks[]; extern const size_t g_ppu_chunk_count;\n"
        "int main(int, char** argv) {\n"
        "    std::string dir = argv[0]; size_t s = dir.find_last_of(\"/\\\\\"); dir = (s == std::string::npos) ? \"\" : dir.substr(0, s + 1);\n"
        "    if (ps3rt_init((dir + \"guest_image.bin\").c_str()) != 0) { std::fprintf(stderr, \"guest_image.bin not found next to the exe\\n\"); return 1; }\n"
        "    PPUContext c; std::memset(&c, 0, sizeof c);\n"
        "    c.mem = ps3rt_memory(); c.pc = kEntryPc; c.gpr[2] = kEntryToc; c.gpr[1] = ps3rt_stack_top();\n"
        "    ppu_run(c, g_ppu_chunks, g_ppu_chunk_count);\n"
        "    std::fprintf(stderr, \"[ps3] PPU halted at pc=0x%llx\\n\", (unsigned long long)c.pc);\n"
        "    ps3rt_shutdown(); return 0;\n}\n";
    if (!write_file(fs::path(dir) / "game_main.cpp", mainSrc)) { err = "Cannot write game_main.cpp"; return false; }

    // guest_image.bin: "PS3IMG1\0", u32 nseg, then per segment {u64 vaddr, u64 filesz, u64 memsz, u64 data_offset}, then data. Little-endian.
    {
        std::string img("PS3IMG1\0", 8);
        auto p32 = [&](uint32_t v) { img.append((const char*)&v, 4); };
        auto p64 = [&](uint64_t v) { img.append((const char*)&v, 8); };
        p32((uint32_t)elf.segments.size());
        uint64_t off = 8 + 4 + elf.segments.size() * 32;
        for (const auto& s : elf.segments) { p64(s.vaddr); p64(s.filesz); p64(s.memsz); p64(off); off += s.filesz; }
        for (const auto& s : elf.segments) img.append((const char*)s.data.data(), s.data.size());
        if (!write_file(fs::path(dir) / "guest_image.bin", img)) { err = "Cannot write guest_image.bin"; return false; }
    }

    std::ostringstream rep;
    rep << "instructions: " << st.instructions << "\nimplemented: " << st.implemented
        << "\nunimplemented: " << st.unimplemented << "\nchunks: " << st.chunks << "\n\nmissing opcodes (most useful TODO list):\n";
    for (auto& kv : st.missing) rep << "  " << kv.first << "  x" << kv.second << "\n";
    write_file(fs::path(dir) / "lift_report.txt", rep.str());
    return true;
}

} // namespace ps3
