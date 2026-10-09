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
    char buf[768];
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
        return ra ? fmt("(c.gpr[%u] + (int64_t)%lld)", ra, (long long)d)
                  : fmt("(uint64_t)%lld", (long long)d);
    };
    auto ea_idx = [&]() {        // (RA|0) + RB
        return ra ? fmt("(c.gpr[%u] + c.gpr[%u])", ra, rb) : fmt("c.gpr[%u]", rb);
    };
    auto rc0 = [&](unsigned r) {
        if (rc) o << fmt(" set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", r);
    };

    switch (op) {

    // ---------- primary opcodes ----------
    case 7:  // mulli
        o << fmt("c.gpr[%u] = (uint64_t)((int64_t)c.gpr[%u] * (int64_t)%lld);", rt, ra, (long long)simm);
        return true;

    case 8:  // subfic
        o << fmt("c.gpr[%u] = (uint64_t)%lld - c.gpr[%u];", rt, (long long)simm, ra);
        // CA bit in XER not modeled yet
        return true;

    case 12: // addic
        o << fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)simm);
        return true;
    case 13: // addic.
        o << fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);",
                 rt, ra, (long long)simm, rt);
        return true;

    case 14: // addi / li
        o << (ra ? fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)simm)
                 : fmt("c.gpr[%u] = (uint64_t)%lld;", rt, (long long)simm));
        return true;
    case 15: // addis / lis
        o << (ra ? fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)(simm << 16))
                 : fmt("c.gpr[%u] = (uint64_t)%lld;", rt, (long long)(simm << 16)));
        return true;

    case 24: o << fmt("c.gpr[%u] = c.gpr[%u] | 0x%llxull;", ra, rt, (unsigned long long)uimm); return true; // ori
    case 25: o << fmt("c.gpr[%u] = c.gpr[%u] | 0x%llxull;", ra, rt, (unsigned long long)(uimm << 16)); return true; // oris
    case 26: o << fmt("c.gpr[%u] = c.gpr[%u] ^ 0x%llxull;", ra, rt, (unsigned long long)uimm); return true; // xori
    case 27: o << fmt("c.gpr[%u] = c.gpr[%u] ^ 0x%llxull;", ra, rt, (unsigned long long)(uimm << 16)); return true; // xoris
    case 28: o << fmt("c.gpr[%u] = c.gpr[%u] & 0x%llxull; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", ra, rt, (unsigned long long)uimm, ra); return true; // andi.
    case 29: o << fmt("c.gpr[%u] = c.gpr[%u] & 0x%llxull; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", ra, rt, (unsigned long long)(uimm << 16), ra); return true; // andis.

    case 32: o << fmt("c.gpr[%u] = rd32(c, %s);", rt, ea(simm).c_str()); return true; // lwz
    case 33: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd32(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // lwzu
    case 34: o << fmt("c.gpr[%u] = rd8(c, %s);", rt, ea(simm).c_str()); return true;  // lbz
    case 35: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd8(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // lbzu
    case 40: o << fmt("c.gpr[%u] = rd16(c, %s);", rt, ea(simm).c_str()); return true; // lhz
    case 41: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd16(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // lhzu
    case 42: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, %s);", rt, ea(simm).c_str()); return true; // lha
    case 43: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // lhau

    case 36: o << fmt("wr32(c, %s, (uint32_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true; // stw
    case 37: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr32(c, a, (uint32_t)c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // stwu
    case 38: o << fmt("wr8(c, %s, (uint8_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true;  // stb
    case 39: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr8(c, a, (uint8_t)c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // stbu
    case 44: o << fmt("wr16(c, %s, (uint16_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true; // sth
    case 45: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr16(c, a, (uint16_t)c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true; // sthu

    case 46: { // lmw
        o << fmt("{ uint64_t a = %s; ", ea(simm).c_str());
        for (unsigned r = rt; r < 32; ++r)
            o << fmt("c.gpr[%u] = rd32(c, a); a += 4; ", r);
        o << "}";
        return true;
    }
    case 47: { // stmw
        o << fmt("{ uint64_t a = %s; ", ea(simm).c_str());
        for (unsigned r = rt; r < 32; ++r)
            o << fmt("wr32(c, a, (uint32_t)c.gpr[%u]); a += 4; ", r);
        o << "}";
        return true;
    }

    case 58: { // DS-form loads
        int64_t ds = (int16_t)(w & 0xFFFC);
        unsigned x = w & 3;
        if (x == 0) { o << fmt("c.gpr[%u] = rd64(c, %s);", rt, ea(ds).c_str()); return true; } // ld
        if (x == 1) { o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd64(c, a); c.gpr[%u] = a; }", ra, (long long)ds, rt, ra); return true; } // ldu
        if (x == 2) { o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int32_t)rd32(c, %s);", rt, ea(ds).c_str()); return true; } // lwa
        break;
    }
    case 62: { // DS-form stores
        int64_t ds = (int16_t)(w & 0xFFFC);
        unsigned x = w & 3;
        if (x == 0) { o << fmt("wr64(c, %s, c.gpr[%u]);", ea(ds).c_str(), rt); return true; } // std
        if (x == 1) { o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr64(c, a, c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)ds, rt, ra); return true; } // stdu
        break;
    }

    case 10: { // cmpli
        unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
        o << (l ? fmt("set_cr_unsigned(c, %u, c.gpr[%u], 0x%llxull);", bf, ra, (unsigned long long)uimm)
                : fmt("set_cr_unsigned(c, %u, (uint32_t)c.gpr[%u], 0x%llxull);", bf, ra, (unsigned long long)uimm));
        return true;
    }
    case 11: { // cmpi
        unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
        o << (l ? fmt("set_cr_signed(c, %u, (int64_t)c.gpr[%u], %lld);", bf, ra, (long long)simm)
                : fmt("set_cr_signed(c, %u, (int32_t)c.gpr[%u], %lld);", bf, ra, (long long)simm));
        return true;
    }

    case 21: { // rlwinm
        unsigned sh = rb, mb = (w >> 6) & 31, me = (w >> 1) & 31;
        uint64_t m = mask64(mb + 32, me + 32);
        o << fmt("{ uint64_t v = (uint32_t)c.gpr[%u]; v = (v << 32) | v; v = (v << %u) | (%u ? (v >> (64 - %u)) : 0); c.gpr[%u] = v & 0x%llxull; }",
                 rt, sh, sh, sh, ra, (unsigned long long)m);
        rc0(ra); return true;
    }

    // 64-bit rotate-left doubleword immediate (very common on Cell)
    case 30: {
        unsigned md = (w >> 2) & 0xF;
        unsigned sh = rb | (((w >> 1) & 1) << 5);
        unsigned mb = ((w >> 6) & 0x1F) | ((w & 0x20));
        unsigned me = mb; // for forms that use me
        // md encoding:
        // 0 = rldicl, 1 = rldicr, 2 = rldic, 3 = rldimi
        // 8 = rldcl,  9 = rldcr  (variable shift – less common)
        if (md == 0) { // rldicl
            uint64_t m = mask64(mb, 63);
            o << fmt("{ uint64_t v = c.gpr[%u]; v = (v << %u) | (%u ? (v >> (64 - %u)) : 0); c.gpr[%u] = v & 0x%llxull; }",
                     rt, sh, sh, sh, ra, (unsigned long long)m);
            rc0(ra); return true;
        }
        if (md == 1) { // rldicr
            me = ((w >> 6) & 0x1F) | ((w & 0x20));
            uint64_t m = mask64(0, me);
            o << fmt("{ uint64_t v = c.gpr[%u]; v = (v << %u) | (%u ? (v >> (64 - %u)) : 0); c.gpr[%u] = v & 0x%llxull; }",
                     rt, sh, sh, sh, ra, (unsigned long long)m);
            rc0(ra); return true;
        }
        if (md == 2) { // rldic
            uint64_t m = mask64(mb, 63 - sh);
            o << fmt("{ uint64_t v = c.gpr[%u]; v = (v << %u) | (%u ? (v >> (64 - %u)) : 0); c.gpr[%u] = v & 0x%llxull; }",
                     rt, sh, sh, sh, ra, (unsigned long long)m);
            rc0(ra); return true;
        }
        if (md == 3) { // rldimi
            uint64_t m = mask64(mb, 63 - sh);
            o << fmt("{ uint64_t v = c.gpr[%u]; v = (v << %u) | (%u ? (v >> (64 - %u)) : 0); c.gpr[%u] = (c.gpr[%u] & ~0x%llxull) | (v & 0x%llxull); }",
                     rt, sh, sh, sh, ra, ra, (unsigned long long)m, (unsigned long long)m);
            rc0(ra); return true;
        }
        key = fmt("op=30 md=%u", md);
        return false;
    }

    case 18: { // b / ba / bl / bla
        int64_t li = ((int32_t)(w & 0x03FFFFFC) << 6) >> 6;
        uint64_t t = (w & 2) ? (uint64_t)li : e.pc + (uint64_t)li;
        if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
        o << e.jump_const(t);
        return true;
    }
    case 16: { // bc
        unsigned bo = rt, bi = ra;
        int64_t bd = (int16_t)(w & 0xFFFC);
        uint64_t t = (w & 2) ? (uint64_t)bd : e.pc + (uint64_t)bd;
        o << fmt("if (bc_taken(c, %u, %u)) { ", bo, bi);
        if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
        o << e.jump_const(t) << " }";
        return true;
    }
    case 17: // sc
        o << fmt("ps3rt_syscall(&c); c.pc = 0x%llxull; return true;", next);
        return true;

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
        // isync, sync-like, CR logicals
        if (xo == 150) { /* isync */ return true; }          // no-op for now
        if (xo == 598) { /* sync  */ return true; }
        if (xo == 854) { /* eieio */ return true; }
        if (xo == 193) { // crxor
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = ((c.cr[%u>>2]>>(3-(%u&3)))&1) ^ ((c.cr[%u>>2]>>(3-(%u&3)))&1); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 449) { // cror
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = ((c.cr[%u>>2]>>(3-(%u&3)))&1) | ((c.cr[%u>>2]>>(3-(%u&3)))&1); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 257) { // crand
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = ((c.cr[%u>>2]>>(3-(%u&3)))&1) & ((c.cr[%u>>2]>>(3-(%u&3)))&1); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 225) { // crnand
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = !(((c.cr[%u>>2]>>(3-(%u&3)))&1) & ((c.cr[%u>>2]>>(3-(%u&3)))&1)); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 289) { // creqv
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = !(((c.cr[%u>>2]>>(3-(%u&3)))&1) ^ ((c.cr[%u>>2]>>(3-(%u&3)))&1)); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 33) { // crnor
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = !(((c.cr[%u>>2]>>(3-(%u&3)))&1) | ((c.cr[%u>>2]>>(3-(%u&3)))&1)); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 417) { // crorc
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = ((c.cr[%u>>2]>>(3-(%u&3)))&1) | !((c.cr[%u>>2]>>(3-(%u&3)))&1); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 129) { // crandc
            unsigned bt = rt, ba = ra, bb = rb;
            o << fmt("{ int f=%u>>2, b=%u&3; bool v = ((c.cr[%u>>2]>>(3-(%u&3)))&1) & !((c.cr[%u>>2]>>(3-(%u&3)))&1); "
                     "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }",
                     bt, bt, ba, ba, bb, bb);
            return true;
        }
        if (xo == 0) { // mcrf
            unsigned bf = (w >> 23) & 7, bfa = (w >> 18) & 7;
            o << fmt("c.cr[%u] = c.cr[%u];", bf, bfa);
            return true;
        }
        key = fmt("op=19 xo=%u", xo);
        return false;
    }

    case 31: {
        unsigned xo = (w >> 1) & 0x3FF;
        switch (xo) {
        // arithmetic
        case 266: o << fmt("c.gpr[%u] = c.gpr[%u] + c.gpr[%u];", rt, ra, rb); rc0(rt); return true; // add
        case 40:  o << fmt("c.gpr[%u] = c.gpr[%u] - c.gpr[%u];", rt, rb, ra); rc0(rt); return true; // subf
        case 10:  o << fmt("c.gpr[%u] = c.gpr[%u] + c.gpr[%u];", rt, ra, rb); rc0(rt); return true; // addc (CA ignored)
        case 8:   o << fmt("c.gpr[%u] = c.gpr[%u] - c.gpr[%u];", rt, rb, ra); rc0(rt); return true; // subfc
        case 138: o << fmt("c.gpr[%u] = c.gpr[%u] + c.gpr[%u];", rt, ra, rb); rc0(rt); return true; // adde
        case 136: o << fmt("c.gpr[%u] = c.gpr[%u] - c.gpr[%u];", rt, rb, ra); rc0(rt); return true; // subfe
        case 202: o << fmt("c.gpr[%u] = c.gpr[%u];", rt, ra); rc0(rt); return true; // addze
        case 200: o << fmt("c.gpr[%u] = ~c.gpr[%u] + 1;", rt, ra); rc0(rt); return true; // subfze (approx)
        case 234: o << fmt("c.gpr[%u] = c.gpr[%u] + (uint64_t)-1;", rt, ra); rc0(rt); return true; // addme
        case 232: o << fmt("c.gpr[%u] = ~c.gpr[%u] + (uint64_t)-1 + 1;", rt, ra); rc0(rt); return true; // subfme
        case 104: o << fmt("c.gpr[%u] = (uint64_t)-(int64_t)c.gpr[%u];", rt, ra); rc0(rt); return true; // neg

        // multiply / divide
        case 235: o << fmt("c.gpr[%u] = (uint64_t)((int64_t)(int32_t)c.gpr[%u] * (int64_t)(int32_t)c.gpr[%u]);", rt, ra, rb); rc0(rt); return true; // mullw
        case 233: o << fmt("c.gpr[%u] = (uint64_t)((int64_t)c.gpr[%u] * (int64_t)c.gpr[%u]);", rt, ra, rb); rc0(rt); return true; // mulld
        case 491: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : (uint64_t)((int32_t)c.gpr[%u] / (int32_t)c.gpr[%u]);", rt, rb, ra, rb); rc0(rt); return true; // divw
        case 489: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : (uint64_t)((int64_t)c.gpr[%u] / (int64_t)c.gpr[%u]);", rt, rb, ra, rb); rc0(rt); return true; // divd
        case 459: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : (uint32_t)c.gpr[%u] / (uint32_t)c.gpr[%u];", rt, rb, ra, rb); rc0(rt); return true; // divwu
        case 457: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : c.gpr[%u] / c.gpr[%u];", rt, rb, ra, rb); rc0(rt); return true; // divdu

        // logical
        case 444: o << fmt("c.gpr[%u] = c.gpr[%u] | c.gpr[%u];", ra, rt, rb); rc0(ra); return true; // or / mr
        case 28:  o << fmt("c.gpr[%u] = c.gpr[%u] & c.gpr[%u];", ra, rt, rb); rc0(ra); return true; // and
        case 316: o << fmt("c.gpr[%u] = c.gpr[%u] ^ c.gpr[%u];", ra, rt, rb); rc0(ra); return true; // xor
        case 476: o << fmt("c.gpr[%u] = ~(c.gpr[%u] | c.gpr[%u]);", ra, rt, rb); rc0(ra); return true; // nand
        case 124: o << fmt("c.gpr[%u] = ~(c.gpr[%u] | c.gpr[%u]);", ra, rt, rb); rc0(ra); return true; // nor / not
        case 60:  o << fmt("c.gpr[%u] = c.gpr[%u] & ~c.gpr[%u];", ra, rt, rb); rc0(ra); return true; // andc
        case 412: o << fmt("c.gpr[%u] = c.gpr[%u] | ~c.gpr[%u];", ra, rt, rb); rc0(ra); return true; // orc
        case 284: o << fmt("c.gpr[%u] = ~(c.gpr[%u] ^ c.gpr[%u]);", ra, rt, rb); rc0(ra); return true; // eqv

        // shifts
        case 24:  o << fmt("c.gpr[%u] = (uint32_t)c.gpr[%u] << (c.gpr[%u] & 0x3f);", ra, rt, rb); rc0(ra); return true; // slw
        case 536: o << fmt("{ unsigned s = c.gpr[%u] & 0x3f; c.gpr[%u] = s > 31 ? 0 : ((uint32_t)c.gpr[%u] >> s); }", rb, ra, rt); rc0(ra); return true; // srw
        case 792: o << fmt("{ unsigned s = c.gpr[%u] & 0x3f; int32_t v = (int32_t)c.gpr[%u]; c.gpr[%u] = (uint64_t)(int64_t)(s > 31 ? (v < 0 ? -1 : 0) : (v >> s)); }", rb, rt, ra); rc0(ra); return true; // sraw
        case 824: { // srawi
            unsigned sh = rb;
            o << fmt("{ int32_t v = (int32_t)c.gpr[%u]; c.gpr[%u] = (uint64_t)(int64_t)(v >> %u); }", rt, ra, sh);
            rc0(ra); return true;
        }
        case 27:  o << fmt("c.gpr[%u] = c.gpr[%u] << (c.gpr[%u] & 0x7f);", ra, rt, rb); rc0(ra); return true; // sld
        case 539: o << fmt("{ unsigned s = c.gpr[%u] & 0x7f; c.gpr[%u] = s > 63 ? 0 : (c.gpr[%u] >> s); }", rb, ra, rt); rc0(ra); return true; // srd
        case 794: o << fmt("{ unsigned s = c.gpr[%u] & 0x7f; int64_t v = (int64_t)c.gpr[%u]; c.gpr[%u] = (uint64_t)(s > 63 ? (v < 0 ? -1 : 0) : (v >> s)); }", rb, rt, ra); rc0(ra); return true; // srad
        case 826: case 827: { // sradi
            unsigned sh = rb | ((xo & 1) << 5);
            o << fmt("{ int64_t v = (int64_t)c.gpr[%u]; c.gpr[%u] = (uint64_t)(v >> %u); }", rt, ra, sh);
            rc0(ra); return true;
        }

        // count leading zeros / extend
        case 26:  o << fmt("{ uint32_t v = (uint32_t)c.gpr[%u]; c.gpr[%u] = v ? (uint32_t)__builtin_clz(v) : 32; }", rt, ra); rc0(ra); return true; // cntlzw
        case 58:  o << fmt("{ uint64_t v = c.gpr[%u]; c.gpr[%u] = v ? (uint64_t)__builtin_clzll(v) : 64; }", rt, ra); rc0(ra); return true; // cntlzd
        case 954: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int8_t)c.gpr[%u];", ra, rt); rc0(ra); return true; // extsb
        case 922: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int16_t)c.gpr[%u];", ra, rt); rc0(ra); return true; // extsh
        case 986: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int32_t)c.gpr[%u];", ra, rt); rc0(ra); return true; // extsw

        // compare
        case 0: case 32: {
            unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
            if (xo == 0)
                o << (l ? fmt("set_cr_signed(c, %u, (int64_t)c.gpr[%u], (int64_t)c.gpr[%u]);", bf, ra, rb)
                        : fmt("set_cr_signed(c, %u, (int32_t)c.gpr[%u], (int32_t)c.gpr[%u]);", bf, ra, rb));
            else
                o << (l ? fmt("set_cr_unsigned(c, %u, c.gpr[%u], c.gpr[%u]);", bf, ra, rb)
                        : fmt("set_cr_unsigned(c, %u, (uint32_t)c.gpr[%u], (uint32_t)c.gpr[%u]);", bf, ra, rb));
            return true;
        }

        // indexed loads / stores
        case 23:  o << fmt("c.gpr[%u] = rd32(c, %s);", rt, ea_idx().c_str()); return true; // lwzx
        case 55:  o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd32(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // lwzux
        case 87:  o << fmt("c.gpr[%u] = rd8(c, %s);", rt, ea_idx().c_str()); return true;  // lbzx
        case 119: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd8(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // lbzux
        case 279: o << fmt("c.gpr[%u] = rd16(c, %s);", rt, ea_idx().c_str()); return true; // lhzx
        case 311: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd16(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // lhzux
        case 343: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, %s);", rt, ea_idx().c_str()); return true; // lhax
        case 375: o << fmt("{ uint64_t a = %s; c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // lhaux
        case 21:  o << fmt("c.gpr[%u] = rd64(c, %s);", rt, ea_idx().c_str()); return true; // ldx
        case 53:  o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd64(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // ldux
        case 341: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int32_t)rd32(c, %s);", rt, ea_idx().c_str()); return true; // lwax
        case 373: o << fmt("{ uint64_t a = %s; c.gpr[%u] = (uint64_t)(int64_t)(int32_t)rd32(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // lwaux

        case 151: o << fmt("wr32(c, %s, (uint32_t)c.gpr[%u]);", ea_idx().c_str(), rt); return true; // stwx
        case 183: o << fmt("{ uint64_t a = %s; wr32(c, a, (uint32_t)c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // stwux
        case 215: o << fmt("wr8(c, %s, (uint8_t)c.gpr[%u]);", ea_idx().c_str(), rt); return true;  // stbx
        case 247: o << fmt("{ uint64_t a = %s; wr8(c, a, (uint8_t)c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // stbux
        case 407: o << fmt("wr16(c, %s, (uint16_t)c.gpr[%u]);", ea_idx().c_str(), rt); return true; // sthx
        case 439: o << fmt("{ uint64_t a = %s; wr16(c, a, (uint16_t)c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // sthux
        case 149: o << fmt("wr64(c, %s, c.gpr[%u]);", ea_idx().c_str(), rt); return true; // stdx
        case 181: o << fmt("{ uint64_t a = %s; wr64(c, a, c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true; // stdux

        // CR / special registers
        case 19:  { // mfcr
            o << fmt("{ uint32_t v=0; for(int i=0;i<8;i++) v = (v<<4) | (c.cr[i] & 0xf); c.gpr[%u] = v; }", rt);
            return true;
        }
        case 144: { // mtcrf
            unsigned fxm = (w >> 12) & 0xFF;
            o << fmt("{ uint32_t v = (uint32_t)c.gpr[%u]; ", rt);
            for (int i = 0; i < 8; ++i)
                if (fxm & (0x80 >> i))
                    o << fmt("c.cr[%d] = (v >> %d) & 0xf; ", i, 28 - 4*i);
            o << "}";
            return true;
        }
        case 339: case 467: { // mfspr / mtspr
            unsigned spr = ((w >> 16) & 31) | (((w >> 11) & 31) << 5);
            const char* r = spr == 8 ? "c.lr" : spr == 9 ? "c.ctr" : spr == 1 ? "c.xer" : nullptr;
            if (!r) { key = fmt("op=31 xo=%u spr=%u", xo, spr); return false; }
            o << (xo == 339 ? fmt("c.gpr[%u] = %s;", rt, r) : fmt("%s = c.gpr[%u];", r, rt));
            return true;
        }

        // cache / barrier – treat as no-ops for now
        case 54: case 86: case 246: case 278: case 1014: // dcbst, dcbf, dcbtst, dcbt, dcbz
        case 598: // sync already handled under 19, but also appears here in some encodings
            return true;

        default:
            key = fmt("op=31 xo=%u", xo);
            return false;
        }
    }

    default:
        break;
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
    std::ostringstream table;
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
            f << "    default: c.pc = pc; c.halted = true; return true;\n    }\n";
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

    uint64_t pc = 0, toc = 0;
    if (!elf.read64(elf.entry, pc) || !elf.read64(elf.entry + 8, toc)) { pc = elf.entry; toc = 0; }
    std::ostringstream info;
    info << "// GENERATED\n#pragma once\n#include <cstdint>\n"
         << fmt("static const uint64_t kEntryPc  = 0x%llxull;\nstatic const uint64_t kEntryToc = 0x%llxull;\n",
                (unsigned long long)pc, (unsigned long long)toc);
    if (!write_file(fs::path(dir) / "image_info.h", info.str())) { err = "Cannot write image_info.h"; return false; }

    static const char* mainSrc =
        "// GENERATED game entry point\n"
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
    // sort by frequency descending for easier prioritisation
    std::vector<std::pair<std::string, uint64_t>> sorted(st.missing.begin(), st.missing.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b){ return a.second > b.second; });
    for (auto& kv : sorted) rep << "  " << kv.first << "  x" << kv.second << "\n";
    write_file(fs::path(dir) / "lift_report.txt", rep.str());
    return true;
}

} // namespace ps3
