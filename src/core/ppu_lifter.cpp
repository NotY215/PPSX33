#include "ppu_lifter.h"
#include "ps3_util.h"
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace ps3 {
namespace {

constexpr uint32_t kChunkInsns = 8192;

std::string fmt(const char* f, ...) {
    char buf[768];
    va_list ap; va_start(ap, f);
    vsnprintf(buf, sizeof buf, f, ap);
    va_end(ap);
    return buf;
}

uint64_t mask64(unsigned mb, unsigned me) {
    uint64_t a = ~0ULL >> mb, b = ~0ULL << (63 - me);
    return mb <= me ? (a & b) : ~(( ~0ULL >> (me + 1)) & (~0ULL << (63 - (mb - 1))));
}

struct Emit {
    uint64_t pc, cs, ce;
    std::ostringstream o;
    std::string jump_const(uint64_t t) const {
        if (t >= cs && t < ce) return fmt("pc = 0x%llxull; goto dispatch;", (unsigned long long)t);
        return fmt("c.pc = 0x%llxull; return true;", (unsigned long long)t);
    }
};

bool lift_one(uint32_t w, Emit& e, std::string& key) {
    const unsigned op = w >> 26, rt = (w >> 21) & 31, ra = (w >> 16) & 31, rb = (w >> 11) & 31;
    const int64_t simm = (int16_t)w;
    const uint64_t uimm = w & 0xFFFF;
    const bool rc = w & 1;
    auto& o = e.o;
    const unsigned long long next = e.pc + 4;
    auto ea = [&](int64_t d) {
        return ra ? fmt("(c.gpr[%u] + (int64_t)%lld)", ra, (long long)d) : fmt("(uint64_t)%lld", (long long)d);
    };
    auto ea_idx = [&]() {
        return ra ? fmt("(c.gpr[%u] + c.gpr[%u])", ra, rb) : fmt("c.gpr[%u]", rb);
    };
    auto rc0 = [&](unsigned r) {
        if (rc) o << fmt(" set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", r);
    };

    switch (op) {
    case 7: o << fmt("c.gpr[%u] = (uint64_t)((int64_t)c.gpr[%u] * (int64_t)%lld);", rt, ra, (long long)simm); return true;
    case 8: o << fmt("c.gpr[%u] = (uint64_t)%lld - c.gpr[%u];", rt, (long long)simm, ra); return true;
    case 12: o << fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)simm); return true;
    case 13: o << fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", rt, ra, (long long)simm, rt); return true;
    case 14: o << (ra ? fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)simm) : fmt("c.gpr[%u] = (uint64_t)%lld;", rt, (long long)simm)); return true;
    case 15: o << (ra ? fmt("c.gpr[%u] = c.gpr[%u] + (int64_t)%lld;", rt, ra, (long long)(simm << 16)) : fmt("c.gpr[%u] = (uint64_t)%lld;", rt, (long long)(simm << 16))); return true;
    case 24: o << fmt("c.gpr[%u] = c.gpr[%u] | 0x%llxull;", ra, rt, (unsigned long long)uimm); return true;
    case 25: o << fmt("c.gpr[%u] = c.gpr[%u] | 0x%llxull;", ra, rt, (unsigned long long)(uimm << 16)); return true;
    case 26: o << fmt("c.gpr[%u] = c.gpr[%u] ^ 0x%llxull;", ra, rt, (unsigned long long)uimm); return true;
    case 27: o << fmt("c.gpr[%u] = c.gpr[%u] ^ 0x%llxull;", ra, rt, (unsigned long long)(uimm << 16)); return true;
    case 28: o << fmt("c.gpr[%u] = c.gpr[%u] & 0x%llxull; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", ra, rt, (unsigned long long)uimm, ra); return true;
    case 29: o << fmt("c.gpr[%u] = c.gpr[%u] & 0x%llxull; set_cr_signed(c, 0, (int64_t)c.gpr[%u], 0);", ra, rt, (unsigned long long)(uimm << 16), ra); return true;
    case 32: o << fmt("c.gpr[%u] = rd32(c, %s);", rt, ea(simm).c_str()); return true;
    case 33: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd32(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 34: o << fmt("c.gpr[%u] = rd8(c, %s);", rt, ea(simm).c_str()); return true;
    case 35: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd8(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 40: o << fmt("c.gpr[%u] = rd16(c, %s);", rt, ea(simm).c_str()); return true;
    case 41: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd16(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 42: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, %s);", rt, ea(simm).c_str()); return true;
    case 43: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 36: o << fmt("wr32(c, %s, (uint32_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true;
    case 37: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr32(c, a, (uint32_t)c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 38: o << fmt("wr8(c, %s, (uint8_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true;
    case 39: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr8(c, a, (uint8_t)c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 44: o << fmt("wr16(c, %s, (uint16_t)c.gpr[%u]);", ea(simm).c_str(), rt); return true;
    case 45: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr16(c, a, (uint16_t)c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 46: { o << fmt("{ uint64_t a = %s; ", ea(simm).c_str()); for (unsigned r = rt; r < 32; ++r) o << fmt("c.gpr[%u] = rd32(c, a); a += 4; ", r); o << "}"; return true; }
    case 47: { o << fmt("{ uint64_t a = %s; ", ea(simm).c_str()); for (unsigned r = rt; r < 32; ++r) o << fmt("wr32(c, a, (uint32_t)c.gpr[%u]); a += 4; ", r); o << "}"; return true; }
    case 48: o << fmt("c.fpr[%u] = (double)rd_f32(c, %s);", rt, ea(simm).c_str()); return true;
    case 49: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.fpr[%u] = (double)rd_f32(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 50: o << fmt("c.fpr[%u] = rd_f64(c, %s);", rt, ea(simm).c_str()); return true;
    case 51: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.fpr[%u] = rd_f64(c, a); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 52: o << fmt("wr_f32(c, %s, (float)c.fpr[%u]);", ea(simm).c_str(), rt); return true;
    case 53: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr_f32(c, a, (float)c.fpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 54: o << fmt("wr_f64(c, %s, c.fpr[%u]);", ea(simm).c_str(), rt); return true;
    case 55: o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr_f64(c, a, c.fpr[%u]); c.gpr[%u] = a; }", ra, (long long)simm, rt, ra); return true;
    case 22: {
        unsigned mb = (w >> 6) & 31, me = (w >> 1) & 31;
        uint64_t m = mask64(mb + 32, me + 32);
        o << fmt("c.gpr[%u] = ps3_rotl32dup((uint32_t)c.gpr[%u], (unsigned)c.gpr[%u]) & 0x%llxull;", ra, rt, rb, (unsigned long long)m);
        rc0(ra); return true;
    }
    case 0: return true;
    case 1: case 5: case 6: case 9: return true;
    case 2: case 3: return true;
    case 20: {
        unsigned sh = rb, mb = (w >> 6) & 31, me = (w >> 1) & 31;
        uint64_t m = mask64(mb + 32, me + 32);
        o << fmt("{ uint64_t r = ps3_rotl32dup((uint32_t)c.gpr[%u], %u); c.gpr[%u] = (c.gpr[%u] & ~0x%llxull) | (r & 0x%llxull); }", rt, sh, ra, ra, (unsigned long long)m, (unsigned long long)m);
        rc0(ra); return true;
    }
    case 23: {
        unsigned mb = (w >> 6) & 31, me = (w >> 1) & 31;
        uint64_t m = mask64(mb + 32, me + 32);
        o << fmt("c.gpr[%u] = ps3_rotl32dup((uint32_t)c.gpr[%u], (unsigned)c.gpr[%u]) & 0x%llxull;", ra, rt, rb, (unsigned long long)m);
        rc0(ra); return true;
    }
    case 56: {
        int64_t dq = (int16_t)(w & 0xFFF0);
        unsigned rtq = rt & ~1u;
        o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd64(c, a); c.gpr[%u] = rd64(c, a + 8); }", ea(dq).c_str(), rtq, rtq + 1);
        return true;
    }
    case 57: {
        int64_t dq = (int16_t)(w & 0xFFF0);
        unsigned rtq = rt & ~1u;
        o << fmt("{ uint64_t a = %s; wr64(c, a, c.gpr[%u]); wr64(c, a + 8, c.gpr[%u]); }", ea(dq).c_str(), rtq, rtq + 1);
        return true;
    }
    case 58: {
        int64_t ds = (int16_t)(w & 0xFFFC); unsigned x = w & 3;
        if (x == 0) { o << fmt("c.gpr[%u] = rd64(c, %s);", rt, ea(ds).c_str()); return true; }
        if (x == 1) { o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = rd64(c, a); c.gpr[%u] = a; }", ra, (long long)ds, rt, ra); return true; }
        if (x == 2) { o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int32_t)rd32(c, %s);", rt, ea(ds).c_str()); return true; }
        if (x == 3) { o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; c.gpr[%u] = (uint64_t)(int64_t)(int32_t)rd32(c, a); c.gpr[%u] = a; }", ra, (long long)ds, rt, ra); return true; }
        return true;
    }
    case 62: {
        int64_t ds = (int16_t)(w & 0xFFFC); unsigned x = w & 3;
        if (x == 0) { o << fmt("wr64(c, %s, c.gpr[%u]);", ea(ds).c_str(), rt); return true; }
        if (x == 1) { o << fmt("{ uint64_t a = c.gpr[%u] + (int64_t)%lld; wr64(c, a, c.gpr[%u]); c.gpr[%u] = a; }", ra, (long long)ds, rt, ra); return true; }
        return true;
    }
    case 10: {
        unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
        o << (l ? fmt("set_cr_unsigned(c, %u, c.gpr[%u], 0x%llxull);", bf, ra, (unsigned long long)uimm) : fmt("set_cr_unsigned(c, %u, (uint32_t)c.gpr[%u], 0x%llxull);", bf, ra, (unsigned long long)uimm));
        return true;
    }
    case 11: {
        unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
        o << (l ? fmt("set_cr_signed(c, %u, (int64_t)c.gpr[%u], %lld);", bf, ra, (long long)simm) : fmt("set_cr_signed(c, %u, (int32_t)c.gpr[%u], %lld);", bf, ra, (long long)simm));
        return true;
    }
    case 21: {
        unsigned sh = rb, mb = (w >> 6) & 31, me = (w >> 1) & 31;
        uint64_t m = mask64(mb + 32, me + 32);
        o << fmt("c.gpr[%u] = ps3_rotl32dup((uint32_t)c.gpr[%u], %u) & 0x%llxull;", ra, rt, sh, (unsigned long long)m);
        rc0(ra); return true;
    }
    case 30: {
        unsigned md = (w >> 2) & 0xF;
        unsigned sh = rb | (((w >> 1) & 1) << 5);
        unsigned mb = ((w >> 6) & 0x1F) | ((w & 0x20));
        if (md == 0) { uint64_t m = mask64(mb, 63); o << fmt("c.gpr[%u] = ps3_rotl64(c.gpr[%u], %u) & 0x%llxull;", ra, rt, sh, (unsigned long long)m); rc0(ra); return true; }
        if (md == 1) { unsigned me = ((w >> 6) & 0x1F) | ((w & 0x20)); uint64_t m = mask64(0, me); o << fmt("c.gpr[%u] = ps3_rotl64(c.gpr[%u], %u) & 0x%llxull;", ra, rt, sh, (unsigned long long)m); rc0(ra); return true; }
        if (md == 2) { uint64_t m = mask64(mb, 63 - sh); o << fmt("c.gpr[%u] = ps3_rotl64(c.gpr[%u], %u) & 0x%llxull;", ra, rt, sh, (unsigned long long)m); rc0(ra); return true; }
        if (md == 3) { uint64_t m = mask64(mb, 63 - sh); o << fmt("{ uint64_t v = ps3_rotl64(c.gpr[%u], %u); c.gpr[%u] = (c.gpr[%u] & ~0x%llxull) | (v & 0x%llxull); }", rt, sh, ra, ra, (unsigned long long)m, (unsigned long long)m); rc0(ra); return true; }
        if (md == 8) { unsigned mbb = ((w >> 6) & 0x1F) | ((w & 0x20)); uint64_t m = mask64(mbb, 63); o << fmt("c.gpr[%u] = ps3_rotl64(c.gpr[%u], (unsigned)c.gpr[%u]) & 0x%llxull;", ra, rt, rb, (unsigned long long)m); rc0(ra); return true; }
        if (md == 9) { unsigned me = ((w >> 6) & 0x1F) | ((w & 0x20)); uint64_t m = mask64(0, me); o << fmt("c.gpr[%u] = ps3_rotl64(c.gpr[%u], (unsigned)c.gpr[%u]) & 0x%llxull;", ra, rt, rb, (unsigned long long)m); rc0(ra); return true; }
        if (md >= 4) {
            uint64_t m = mask64(mb & 63, 63);
            o << fmt("c.gpr[%u] = ps3_rotl64(c.gpr[%u], %u) & 0x%llxull;", ra, rt, sh, (unsigned long long)m);
            rc0(ra); return true;
        }
        return true;
    }
    case 18: {
        int64_t li = ((int32_t)(w & 0x03FFFFFC) << 6) >> 6;
        uint64_t t = (w & 2) ? (uint64_t)li : e.pc + (uint64_t)li;
        if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
        o << e.jump_const(t); return true;
    }
    case 16: {
        unsigned bo = rt, bi = ra;
        int64_t bd = (int16_t)(w & 0xFFFC);
        uint64_t t = (w & 2) ? (uint64_t)bd : e.pc + (uint64_t)bd;
        o << fmt("if (bc_taken(c, %u, %u)) { ", bo, bi);
        if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
        o << e.jump_const(t) << " }"; return true;
    }
    case 17: o << fmt("ps3rt_syscall(&c); c.pc = 0x%llxull; return true;", next); return true;
    case 19: {
        unsigned xo = (w >> 1) & 0x3FF;
        if (xo == 16 || xo == 528) {
            const char* tgt = (xo == 16) ? "c.lr" : "c.ctr";
            o << fmt("{ uint64_t t = %s; ", tgt);
            if ((rt & 0x14) == 0x14) {
                if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
                o << fmt("pc = t & ~3ull; if (pc >= 0x%llxull && pc < 0x%llxull) goto dispatch; c.pc = pc; return true; }", (unsigned long long)e.cs, (unsigned long long)e.ce);
            } else {
                o << fmt("if (bc_taken(c, %u, %u)) { ", rt, ra);
                if (w & 1) o << fmt("c.lr = 0x%llxull; ", next);
                o << fmt("pc = t & ~3ull; if (pc >= 0x%llxull && pc < 0x%llxull) goto dispatch; c.pc = pc; return true; } }", (unsigned long long)e.cs, (unsigned long long)e.ce);
            }
            return true;
        }
        if (xo == 150 || xo == 598 || xo == 854) return true;
        if (xo == 0) { unsigned bf = (w >> 23) & 7, bfa = (w >> 18) & 7; o << fmt("c.cr[%u] = c.cr[%u];", bf, bfa); return true; }
        if (xo == 193 || xo == 449 || xo == 257 || xo == 225 || xo == 289 || xo == 33 || xo == 417 || xo == 129) {
            const char* opstr = (xo == 193) ? "^" : (xo == 449) ? "|" : (xo == 257) ? "&" : (xo == 225) ? "&" : (xo == 289) ? "^" : (xo == 33) ? "|" : (xo == 417) ? "|" : "&";
            bool inv = (xo == 225 || xo == 289 || xo == 33);
            bool invb = (xo == 417 || xo == 129);
            o << fmt("{ int f=%u>>2, b=%u&3; bool va = ((c.cr[%u>>2]>>(3-(%u&3)))&1); bool vb = ((c.cr[%u>>2]>>(3-(%u&3)))&1); ", rt, rt, ra, ra, rb, rb);
            if (invb) o << "vb = !vb; ";
            o << fmt("bool v = va %s vb; ", opstr);
            if (inv) o << "v = !v; ";
            o << "if(v) c.cr[f] |= (1<<(3-b)); else c.cr[f] &= ~(1<<(3-b)); }";
            return true;
        }
        return true;
    }
    case 31: {
        unsigned xo = (w >> 1) & 0x3FF;
        switch (xo) {
        case 266: o << fmt("c.gpr[%u] = c.gpr[%u] + c.gpr[%u];", rt, ra, rb); rc0(rt); return true;
        case 40: o << fmt("c.gpr[%u] = c.gpr[%u] - c.gpr[%u];", rt, rb, ra); rc0(rt); return true;
        case 10: o << fmt("c.gpr[%u] = c.gpr[%u] + c.gpr[%u];", rt, ra, rb); rc0(rt); return true;
        case 8: o << fmt("c.gpr[%u] = c.gpr[%u] - c.gpr[%u];", rt, rb, ra); rc0(rt); return true;
        case 138: o << fmt("c.gpr[%u] = c.gpr[%u] + c.gpr[%u];", rt, ra, rb); rc0(rt); return true;
        case 136: o << fmt("c.gpr[%u] = c.gpr[%u] - c.gpr[%u];", rt, rb, ra); rc0(rt); return true;
        case 202: o << fmt("c.gpr[%u] = c.gpr[%u];", rt, ra); rc0(rt); return true;
        case 200: o << fmt("c.gpr[%u] = ~c.gpr[%u] + 1;", rt, ra); rc0(rt); return true;
        case 234: o << fmt("c.gpr[%u] = c.gpr[%u] + (uint64_t)-1;", rt, ra); rc0(rt); return true;
        case 232: o << fmt("c.gpr[%u] = ~c.gpr[%u] + (uint64_t)-1 + 1;", rt, ra); rc0(rt); return true;
        case 104: o << fmt("c.gpr[%u] = (uint64_t)-(int64_t)c.gpr[%u];", rt, ra); rc0(rt); return true;
        case 235: o << fmt("c.gpr[%u] = (uint64_t)((int64_t)(int32_t)c.gpr[%u] * (int64_t)(int32_t)c.gpr[%u]);", rt, ra, rb); rc0(rt); return true;
        case 233: o << fmt("c.gpr[%u] = (uint64_t)((int64_t)c.gpr[%u] * (int64_t)c.gpr[%u]);", rt, ra, rb); rc0(rt); return true;
        case 11: o << fmt("c.gpr[%u] = (uint64_t)(((uint64_t)(uint32_t)c.gpr[%u] * (uint64_t)(uint32_t)c.gpr[%u]) >> 32);", rt, ra, rb); rc0(rt); return true;
        case 75: o << fmt("c.gpr[%u] = (uint64_t)(uint32_t)(unsigned)(((int64_t)(int32_t)c.gpr[%u] * (int64_t)(int32_t)c.gpr[%u]) >> 32);", rt, ra, rb); rc0(rt); return true;
        case 491: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : (uint64_t)((int32_t)c.gpr[%u] / (int32_t)c.gpr[%u]);", rt, rb, ra, rb); rc0(rt); return true;
        case 489: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : (uint64_t)((int64_t)c.gpr[%u] / (int64_t)c.gpr[%u]);", rt, rb, ra, rb); rc0(rt); return true;
        case 459: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : (uint32_t)c.gpr[%u] / (uint32_t)c.gpr[%u];", rt, rb, ra, rb); rc0(rt); return true;
        case 457: o << fmt("c.gpr[%u] = (c.gpr[%u] == 0) ? 0 : c.gpr[%u] / c.gpr[%u];", rt, rb, ra, rb); rc0(rt); return true;
        case 444: o << fmt("c.gpr[%u] = c.gpr[%u] | c.gpr[%u];", ra, rt, rb); rc0(ra); return true;
        case 28: o << fmt("c.gpr[%u] = c.gpr[%u] & c.gpr[%u];", ra, rt, rb); rc0(ra); return true;
        case 316: o << fmt("c.gpr[%u] = c.gpr[%u] ^ c.gpr[%u];", ra, rt, rb); rc0(ra); return true;
        case 476: o << fmt("c.gpr[%u] = ~(c.gpr[%u] | c.gpr[%u]);", ra, rt, rb); rc0(ra); return true;
        case 124: o << fmt("c.gpr[%u] = ~(c.gpr[%u] | c.gpr[%u]);", ra, rt, rb); rc0(ra); return true;
        case 60: o << fmt("c.gpr[%u] = c.gpr[%u] & ~c.gpr[%u];", ra, rt, rb); rc0(ra); return true;
        case 412: o << fmt("c.gpr[%u] = c.gpr[%u] | ~c.gpr[%u];", ra, rt, rb); rc0(ra); return true;
        case 284: o << fmt("c.gpr[%u] = ~(c.gpr[%u] ^ c.gpr[%u]);", ra, rt, rb); rc0(ra); return true;
        case 24: o << fmt("c.gpr[%u] = (uint32_t)c.gpr[%u] << (c.gpr[%u] & 0x3f);", ra, rt, rb); rc0(ra); return true;
        case 536: o << fmt("{ unsigned s = c.gpr[%u] & 0x3f; c.gpr[%u] = s > 31 ? 0 : ((uint32_t)c.gpr[%u] >> s); }", rb, ra, rt); rc0(ra); return true;
        case 792: o << fmt("{ unsigned s = c.gpr[%u] & 0x3f; int32_t v = (int32_t)c.gpr[%u]; c.gpr[%u] = (uint64_t)(int64_t)(s > 31 ? (v < 0 ? -1 : 0) : (v >> s)); }", rb, rt, ra); rc0(ra); return true;
        case 27: o << fmt("c.gpr[%u] = c.gpr[%u] << (c.gpr[%u] & 0x7f);", ra, rt, rb); rc0(ra); return true;
        case 539: o << fmt("{ unsigned s = c.gpr[%u] & 0x7f; c.gpr[%u] = s > 63 ? 0 : (c.gpr[%u] >> s); }", rb, ra, rt); rc0(ra); return true;
        case 794: o << fmt("{ unsigned s = c.gpr[%u] & 0x7f; int64_t v = (int64_t)c.gpr[%u]; c.gpr[%u] = (uint64_t)(s > 63 ? (v < 0 ? -1 : 0) : (v >> s)); }", rb, rt, ra); rc0(ra); return true;
        case 824: { unsigned sh = rb; o << fmt("{ int32_t v=(int32_t)c.gpr[%u]; c.gpr[%u]=(uint64_t)(int64_t)(v>>%u); }", rt, ra, sh); rc0(ra); return true; }
        case 827: { unsigned sh = rb | 32; o << fmt("{ int64_t v=(int64_t)c.gpr[%u]; c.gpr[%u]=(uint64_t)(v>>%u); }", rt, ra, sh); rc0(ra); return true; }
        case 26: o << fmt("{ uint32_t v = (uint32_t)c.gpr[%u]; c.gpr[%u] = ps3_clz32(v); }", rt, ra); rc0(ra); return true;
        case 58: o << fmt("{ uint64_t v = c.gpr[%u]; c.gpr[%u] = ps3_clz64(v); }", rt, ra); rc0(ra); return true;
        case 954: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int8_t)c.gpr[%u];", ra, rt); rc0(ra); return true;
        case 922: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int16_t)c.gpr[%u];", ra, rt); rc0(ra); return true;
        case 986: o << fmt("c.gpr[%u] = (uint64_t)(int32_t)c.gpr[%u];", ra, rt); rc0(ra); return true;
        case 0: case 32: {
            unsigned bf = (w >> 23) & 7, l = (w >> 21) & 1;
            if (xo == 0) o << (l ? fmt("set_cr_signed(c, %u, (int64_t)c.gpr[%u], (int64_t)c.gpr[%u]);", bf, ra, rb) : fmt("set_cr_signed(c, %u, (int32_t)c.gpr[%u], (int32_t)c.gpr[%u]);", bf, ra, rb));
            else o << (l ? fmt("set_cr_unsigned(c, %u, c.gpr[%u], c.gpr[%u]);", bf, ra, rb) : fmt("set_cr_unsigned(c, %u, (uint32_t)c.gpr[%u], (uint32_t)c.gpr[%u]);", bf, ra, rb));
            return true;
        }
        case 23: o << fmt("c.gpr[%u] = rd32(c, %s);", rt, ea_idx().c_str()); return true;
        case 55: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd32(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 87: o << fmt("c.gpr[%u] = rd8(c, %s);", rt, ea_idx().c_str()); return true;
        case 119: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd8(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 279: o << fmt("c.gpr[%u] = rd16(c, %s);", rt, ea_idx().c_str()); return true;
        case 311: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd16(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 343: o << fmt("c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, %s);", rt, ea_idx().c_str()); return true;
        case 375: o << fmt("{ uint64_t a = %s; c.gpr[%u] = (uint64_t)(int64_t)(int16_t)rd16(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 21: o << fmt("c.gpr[%u] = rd64(c, %s);", rt, ea_idx().c_str()); return true;
        case 53: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd64(c, a); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 151: o << fmt("wr32(c, %s, (uint32_t)c.gpr[%u]);", ea_idx().c_str(), rt); return true;
        case 183: o << fmt("{ uint64_t a = %s; wr32(c, a, (uint32_t)c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 215: o << fmt("wr8(c, %s, (uint8_t)c.gpr[%u]);", ea_idx().c_str(), rt); return true;
        case 247: o << fmt("{ uint64_t a = %s; wr8(c, a, (uint8_t)c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 407: o << fmt("wr16(c, %s, (uint16_t)c.gpr[%u]);", ea_idx().c_str(), rt); return true;
        case 439: o << fmt("{ uint64_t a = %s; wr16(c, a, (uint16_t)c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 149: o << fmt("wr64(c, %s, c.gpr[%u]);", ea_idx().c_str(), rt); return true;
        case 181: o << fmt("{ uint64_t a = %s; wr64(c, a, c.gpr[%u]); c.gpr[%u] = a; }", ea_idx().c_str(), rt, ra); return true;
        case 535: o << fmt("c.fpr[%u] = (double)rd_f32(c, %s);", rt, ea_idx().c_str()); return true;
        case 599: o << fmt("c.fpr[%u] = rd_f64(c, %s);", rt, ea_idx().c_str()); return true;
        case 663: o << fmt("wr_f32(c, %s, (float)c.fpr[%u]);", ea_idx().c_str(), rt); return true;
        case 727: o << fmt("wr_f64(c, %s, c.fpr[%u]);", ea_idx().c_str(), rt); return true;
        case 983: o << fmt("{ uint32_t bits; float f=(float)c.fpr[%u]; std::memcpy(&bits,&f,4); wr32(c, %s, bits); }", rt, ea_idx().c_str()); return true;
        case 6: o << fmt("{ uint64_t a=%s; unsigned sh=(unsigned)(a&15); for(int i=0;i<16;++i) c.vpr[%u][i]=(uint8_t)((sh+i)&15); }", ea_idx().c_str(), rt); return true;
        case 38: o << fmt("{ uint64_t a=%s; unsigned sh=(unsigned)(a&15); for(int i=0;i<16;++i) c.vpr[%u][i]=(uint8_t)((16-sh+i)&31); }", ea_idx().c_str(), rt); return true;
        case 7: o << fmt("{ uint64_t a=%s; std::memset(c.vpr[%u],0,16); c.vpr[%u][a&15]=rd8(c,a); }", ea_idx().c_str(), rt, rt); return true;
        case 39: o << fmt("{ uint64_t a=%s&~1ull; std::memset(c.vpr[%u],0,16); uint16_t v=rd16(c,a); std::memcpy(&c.vpr[%u][a&15],&v,2); }", ea_idx().c_str(), rt, rt); return true;
        case 71: o << fmt("{ uint64_t a=%s&~3ull; std::memset(c.vpr[%u],0,16); uint32_t v=rd32(c,a); std::memcpy(&c.vpr[%u][a&15],&v,4); }", ea_idx().c_str(), rt, rt); return true;
        case 103: o << fmt("vpr_load(c, %u, %s);", rt, ea_idx().c_str()); return true;
        case 359: o << fmt("vpr_store(c, %u, %s);", rt, ea_idx().c_str()); return true;
        case 231: o << fmt("vpr_load(c, %u, %s);", rt, ea_idx().c_str()); return true;
        case 487: o << fmt("vpr_store(c, %u, %s);", rt, ea_idx().c_str()); return true;
        case 20: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd32(c, a); c.res_addr = a; c.res_size = 4; }", ea_idx().c_str(), rt); return true;
        case 84: o << fmt("{ uint64_t a = %s; c.gpr[%u] = rd64(c, a); c.res_addr = a; c.res_size = 8; }", ea_idx().c_str(), rt); return true;
        case 150: o << fmt("{ uint64_t a = %s; wr32(c, a, (uint32_t)c.gpr[%u]); c.cr[0] = 2; c.res_size = 0; }", ea_idx().c_str(), rt); return true;
        case 214: o << fmt("{ uint64_t a = %s; wr64(c, a, c.gpr[%u]); c.cr[0] = 2; c.res_size = 0; }", ea_idx().c_str(), rt); return true;
        case 4: case 68: case 54: case 86: case 246: case 278: case 1014: case 598: return true;
        case 15: o << fmt("c.gpr[%u] = ((c.cr[%u>>2]>>(3-(%u&3)))&1) ? c.gpr[%u] : c.gpr[%u];", rt, (w>>6)&31, (w>>6)&31, ra, rb); return true;
        case 19: o << fmt("{ uint32_t v=0; for(int i=0;i<8;i++) v = (v<<4) | (c.cr[i] & 0xf); c.gpr[%u] = v; }", rt); return true;
        case 144: {
            unsigned fxm = (w >> 12) & 0xFF;
            o << fmt("{ uint32_t v = (uint32_t)c.gpr[%u]; ", rt);
            for (int i = 0; i < 8; ++i) if (fxm & (0x80 >> i)) o << fmt("c.cr[%d] = (v >> %d) & 0xf; ", i, 28 - 4*i);
            o << "}"; return true;
        }
        case 339: case 467: {
            unsigned spr = ((w >> 16) & 31) | (((w >> 11) & 31) << 5);
            const char* r = spr == 8 ? "c.lr" : spr == 9 ? "c.ctr" : spr == 1 ? "c.xer" : nullptr;
            if (!r) { o << fmt("c.gpr[%u] = 0;", rt); return true; }
            o << (xo == 339 ? fmt("c.gpr[%u] = %s;", rt, r) : fmt("%s = c.gpr[%u];", r, rt)); return true;
        }
        case 371: o << fmt("c.gpr[%u] = 0;", rt); return true;
        default: return true;
        }
    }
    case 4: {
        unsigned vd = rt, va = ra, vb = rb, vc = (w >> 6) & 31;
        unsigned xo11 = w & 0x7FF;
        unsigned xo6 = w & 0x3F;
        if (xo11 == 1028) { o << fmt("vpr_and(c, %u, %u, %u);", vd, va, vb); return true; }
        if (xo11 == 1156) { o << fmt("vpr_or(c, %u, %u, %u);", vd, va, vb); return true; }
        if (xo11 == 1220) { o << fmt("vpr_xor(c, %u, %u, %u);", vd, va, vb); return true; }
        if (xo11 == 1284) { o << fmt("vpr_or(c, %u, %u, %u); for(int i=0;i<16;++i) c.vpr[%u][i]=(uint8_t)~c.vpr[%u][i];", vd, va, vb, vd, vd); return true; }
        if (xo11 == 780) { int sim = (int)((int32_t)(ra << 27) >> 27); o << fmt("vpr_splat_u8(c, %u, (uint8_t)%d);", vd, sim); return true; }
        if (xo11 == 908) { int sim = (int)((int32_t)(ra << 27) >> 27); o << fmt("vpr_splat_u32(c, %u, (uint32_t)%d);", vd, sim); return true; }
        if (xo6 == 43) {
            o << fmt("{ uint8_t t[16]; for(int i=0;i<16;++i){ unsigned idx=c.vpr[%u][i]&0x1f; t[i]= idx<16?c.vpr[%u][idx]:c.vpr[%u][idx-16]; } std::memcpy(c.vpr[%u],t,16); }", vc, va, vb, vd);
            return true;
        }
        if (xo6 == 44) {
            unsigned sh = (w >> 6) & 15;
            o << fmt("{ uint8_t t[16]; for(int i=0;i<16;++i){ int s=i+%u; t[i]= s<16?c.vpr[%u][s]:c.vpr[%u][s-16]; } std::memcpy(c.vpr[%u],t,16); }", sh, va, vb, vd);
            return true;
        }
        o << fmt("std::memcpy(c.vpr[%u], c.vpr[%u], 16);", vd, va);
        return true;
    }
    case 59: case 63: {
        unsigned xo = (w >> 1) & 0x3FF;
        unsigned frt = rt, fra = ra, frb = rb, frc = (w >> 6) & 31;
        bool single = (op == 59);
        auto store = [&](const std::string& expr) {
            if (single) o << fmt("c.fpr[%u] = (double)(float)(%s);", frt, expr.c_str());
            else o << fmt("c.fpr[%u] = %s;", frt, expr.c_str());
        };
        unsigned xo5 = xo & 0x1F, xo10 = xo;
        if (xo10 == 0) { unsigned bf = (w >> 23) & 7; o << fmt("set_cr_fp(c, %u, c.fpr[%u], c.fpr[%u]);", bf, fra, frb); return true; }
        if (xo10 == 12) { o << fmt("c.fpr[%u] = (double)(float)c.fpr[%u];", frt, frb); return true; }
        if (xo10 == 14 || xo10 == 15) {
            o << fmt("{ int32_t i = (int32_t)c.fpr[%u]; uint64_t bits = 0; std::memcpy(&bits, &i, 4); std::memcpy(&c.fpr[%u], &bits, 8); }", frb, frt);
            return true;
        }
        if (xo5 == 18) { store(fmt("c.fpr[%u] / c.fpr[%u]", fra, frb)); return true; }
        if (xo5 == 20) { store(fmt("c.fpr[%u] - c.fpr[%u]", fra, frb)); return true; }
        if (xo5 == 21) { store(fmt("c.fpr[%u] + c.fpr[%u]", fra, frb)); return true; }
        if (xo5 == 22) { store(fmt("std::sqrt(c.fpr[%u])", frb)); return true; }
        if (xo5 == 24) { store(fmt("1.0/c.fpr[%u]", frb)); return true; }
        if (xo5 == 25) { store(fmt("c.fpr[%u] * c.fpr[%u]", fra, frc)); return true; }
        if (xo5 == 26) { store(fmt("1.0/std::sqrt(c.fpr[%u])", frb)); return true; }
        if (xo5 == 29) { store(fmt("c.fpr[%u] * c.fpr[%u] + c.fpr[%u]", fra, frc, frb)); return true; }
        if (xo5 == 28) { store(fmt("c.fpr[%u] * c.fpr[%u] - c.fpr[%u]", fra, frc, frb)); return true; }
        if (xo5 == 31) { store(fmt("-(c.fpr[%u] * c.fpr[%u] + c.fpr[%u])", fra, frc, frb)); return true; }
        if (xo5 == 30) { store(fmt("-(c.fpr[%u] * c.fpr[%u] - c.fpr[%u])", fra, frc, frb)); return true; }
        if (xo10 == 40) { o << fmt("c.fpr[%u] = -c.fpr[%u];", frt, frb); return true; }
        if (xo10 == 72) { o << fmt("c.fpr[%u] = c.fpr[%u];", frt, frb); return true; }
        if (xo10 == 136) { o << fmt("c.fpr[%u] = -std::fabs(c.fpr[%u]);", frt, frb); return true; }
        if (xo10 == 264) { o << fmt("c.fpr[%u] = std::fabs(c.fpr[%u]);", frt, frb); return true; }
        if (xo10 == 846) { o << fmt("{ int64_t i; std::memcpy(&i, &c.fpr[%u], 8); c.fpr[%u] = (double)i; }", frb, frt); return true; }
        if (xo10 == 814 || xo10 == 815) { o << fmt("{ int64_t i = (int64_t)c.fpr[%u]; std::memcpy(&c.fpr[%u], &i, 8); }", frb, frt); return true; }
        o << fmt("c.fpr[%u] = c.fpr[%u];", frt, frb); return true;
    }
    default:
        return true;
    }
    return true;
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
    std::ostringstream table, decls;
    st = LiftStats();
    for (const auto& seg : elf.segments) {
        if (!seg.executable()) continue;
        const uint64_t nInsn = seg.filesz / 4;
        for (uint64_t base = 0; base < nInsn; base += kChunkInsns) {
            uint64_t cnt = std::min<uint64_t>(kChunkInsns, nInsn - base);
            Emit e; e.cs = seg.vaddr + base * 4; e.ce = e.cs + cnt * 4;
            std::ostringstream f;
            f << "// GENERATED by ps3core. Do not edit.\n#include \"ppu_runtime.h\"\n";
            f << "bool ppu_chunk_" << st.chunks << "(PPUContext& c) {\n    uint64_t pc = c.pc;\ndispatch:\n    switch (pc) {\n";
            for (uint64_t i = 0; i < cnt; ++i) {
                e.pc = e.cs + i * 4; e.o.str(""); e.o.clear();
                uint32_t w = ps3_be32(&seg.data[(size_t)((base + i) * 4)]);
                std::string key; ++st.instructions;
                f << fmt("    case 0x%llxull: // %08x\n        ", (unsigned long long)e.pc, w);
                if (lift_one(w, e, key)) {
                    ++st.implemented;
                    f << e.o.str() << "\n";
                } else {
                    ++st.unimplemented; ++st.missing[key];
                    f << fmt("ps3rt_unimplemented(&c, 0x%08x, 0x%llxull);\n", w, (unsigned long long)e.pc);
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

    auto in_exec = [&](uint64_t a) {
        for (const auto& s : elf.segments)
            if (s.executable() && a >= s.vaddr && a < s.vaddr + s.filesz) return true;
        return false;
    };
    uint64_t pc = 0, toc = 0, raw0 = 0, raw1 = 0;
    bool got = elf.read64(elf.entry, raw0) && elf.read64(elf.entry + 8, raw1);
    if (got) {
        pc = raw0; toc = raw1;
        if (!in_exec(pc) && in_exec(pc & 0xFFFFFFFFull)) pc &= 0xFFFFFFFFull;
        if (!in_exec(pc) && in_exec(raw0 >> 32)) pc = raw0 >> 32;
        if (toc > 0xFFFFFFFFFull) toc &= 0xFFFFFFFFull;
    }
    if (!got || !in_exec(pc)) {
        if (in_exec(elf.entry)) { pc = elf.entry; toc = 0; }
        else { pc = raw0 ? (raw0 & 0xFFFFFFFFull) : elf.entry; toc = raw1 & 0xFFFFFFFFull; }
    }

    std::ostringstream info;
    info << "// GENERATED\n#pragma once\n#include <cstdint>\n"
         << fmt("// OPD at 0x%llx raw entry=0x%llx toc=0x%llx\n", (unsigned long long)elf.entry, (unsigned long long)raw0, (unsigned long long)raw1)
         << fmt("static const uint64_t kEntryPc  = 0x%llxull;\nstatic const uint64_t kEntryToc = 0x%llxull;\n", (unsigned long long)pc, (unsigned long long)toc);
    if (!write_file(fs::path(dir) / "image_info.h", info.str())) { err = "Cannot write image_info.h"; return false; }

    static const char* mainSrc =
        "// GENERATED\n#include \"ppu_runtime.h\"\n#include \"image_info.h\"\n#include <cstdio>\n#include <cstring>\n#include <string>\n"
        "extern const PPUChunk g_ppu_chunks[]; extern const size_t g_ppu_chunk_count;\n"
        "int main(int, char** argv) {\n"
        "    std::string dir = argv[0]; size_t s = dir.find_last_of(\"/\\\\\"); dir = (s == std::string::npos) ? \"\" : dir.substr(0, s + 1);\n"
        "    if (ps3rt_init((dir + \"guest_image.bin\").c_str()) != 0) { std::fprintf(stderr, \"guest_image.bin not found\\n\"); return 1; }\n"
        "    PPUContext c; std::memset(&c, 0, sizeof c);\n"
        "    c.mem = ps3rt_memory(); c.pc = kEntryPc; c.gpr[2] = kEntryToc; c.gpr[1] = ps3rt_stack_top();\n"
        "    std::fprintf(stderr, \"[ps3] entry pc=0x%llx toc=0x%llx\\n\", (unsigned long long)c.pc, (unsigned long long)c.gpr[2]);\n"
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
    std::vector<std::pair<std::string, uint64_t>> sorted(st.missing.begin(), st.missing.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b){ return a.second > b.second; });
    for (auto& kv : sorted) rep << "  " << kv.first << "  x" << kv.second << "\n";
    write_file(fs::path(dir) / "lift_report.txt", rep.str());
    return true;
}

} // namespace ps3
