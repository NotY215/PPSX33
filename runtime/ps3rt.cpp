// ps3rt - runtime library (ps3rt.dll)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>
#include <algorithm>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/mman.h>
  #include <unistd.h>
#endif

namespace {
constexpr uint64_t kGuestSize = 1ull << 32;
constexpr uint64_t kStackBase = 0xD0000000ull;
constexpr uint64_t kStackSize = 2ull << 20;
constexpr uint64_t kHeapBase  = 0x30000000ull;
constexpr uint64_t kHeapSize  = 0x08000000ull;
uint8_t* g_mem = nullptr;
uint64_t g_heap_next = kHeapBase;

std::mutex g_spu_mu;
constexpr int kMaxSpu = 8;
SPUContext g_spu[kMaxSpu];
bool g_spu_used[kMaxSpu] = {};

bool map_region(uint64_t addr, uint64_t size) {
    if (!g_mem || size == 0) return false;
    if (addr >= kGuestSize) return false;
    if (addr + size > kGuestSize) size = kGuestSize - addr;
#if defined(_WIN32)
    uint64_t page = addr & ~0xFFFFull;
    uint64_t end  = (addr + size + 0xFFFF) & ~0xFFFFull;
    return VirtualAlloc(g_mem + page, (SIZE_T)(end - page), MEM_COMMIT, PAGE_READWRITE) != nullptr;
#else
    (void)addr; (void)size; return true;
#endif
}

// ---- SPU helpers (prefer word 0 / preferred slot for scalar-like ops) ----
static inline uint32_t spu_pref(const SPUContext& s, unsigned r) {
    uint32_t v; std::memcpy(&v, s.gpr[r & 127], 4); return bs32(v);
}
static inline void spu_set_pref(SPUContext& s, unsigned r, uint32_t v) {
    uint32_t be = bs32(v);
    std::memset(s.gpr[r & 127], 0, 16);
    std::memcpy(s.gpr[r & 127], &be, 4);
}
static inline void spu_copy_reg(SPUContext& s, unsigned d, unsigned a) {
    std::memcpy(s.gpr[d & 127], s.gpr[a & 127], 16);
}
static inline uint32_t spu_ls_u32(SPUContext& s, uint32_t addr) {
    addr &= 0x3FFFC;
    uint32_t v; std::memcpy(&v, s.ls + addr, 4); return bs32(v);
}
static inline void spu_ls_w32(SPUContext& s, uint32_t addr, uint32_t v) {
    addr &= 0x3FFFC;
    v = bs32(v);
    std::memcpy(s.ls + addr, &v, 4);
}
static inline void spu_ls_load16(SPUContext& s, unsigned rt, uint32_t addr) {
    addr &= 0x3FFF0;
    std::memcpy(s.gpr[rt & 127], s.ls + addr, 16);
}
static inline void spu_ls_store16(SPUContext& s, unsigned rt, uint32_t addr) {
    addr &= 0x3FFF0;
    std::memcpy(s.ls + addr, s.gpr[rt & 127], 16);
}

// Channel numbers (subset)
enum {
    SPU_WrOutMbox = 28,
    SPU_RdInMbox = 29,
    SPU_WrOutIntrMbox = 30,
    MFC_LSA = 16,
    MFC_EAH = 17,
    MFC_EAL = 18,
    MFC_Size = 19,
    MFC_TagID = 20,
    MFC_Cmd = 21,
    SPU_RdEventStat = 0,
    SPU_WrEventMask = 1,
    SPU_RdEventMask = 2,
    SPU_RdSigNotify1 = 3,
    SPU_RdSigNotify2 = 4,
    SPU_WrDec = 7,
    SPU_RdDec = 8,
};

static void spu_channel_write(SPUContext& s, unsigned ch, uint32_t val) {
    switch (ch) {
    case SPU_WrOutMbox:
    case SPU_WrOutIntrMbox:
        if (s.mailbox_out_count < 4)
            s.mailbox_out[s.mailbox_out_count++] = val;
        break;
    case MFC_LSA: s.mfc_lsa = val & 0x3FFFF; break;
    case MFC_EAH: s.mfc_ea = (s.mfc_ea & 0xFFFFFFFFull) | ((uint64_t)val << 32); break;
    case MFC_EAL: s.mfc_ea = (s.mfc_ea & ~0xFFFFFFFFull) | val; break;
    case MFC_Size: s.mfc_size = val & 0x7FFF; break;
    case MFC_TagID: s.mfc_tag = val & 0x1F; break;
    case MFC_Cmd:
        s.mfc_cmd = val;
        // Trigger simplified DMA against main memory when available
        if (g_mem && s.mfc_size) {
            uint32_t lsa = s.mfc_lsa & 0x3FFFF;
            uint64_t ea = s.mfc_ea;
            uint32_t sz = s.mfc_size;
            if ((uint64_t)lsa + sz <= sizeof(s.ls) && ea < kGuestSize) {
                map_region(ea, sz);
                // put* commands have bit patterns with 0x40; get* with 0x20
                if (val & 0x40)
                    std::memcpy(g_mem + ea, s.ls + lsa, sz);
                else
                    std::memcpy(s.ls + lsa, g_mem + ea, sz);
            }
        }
        break;
    case SPU_WrEventMask: s.ch_event_mask = val; break;
    case SPU_WrDec: s.decrementer = val; break;
    default: break;
    }
}

static uint32_t spu_channel_read(SPUContext& s, unsigned ch) {
    switch (ch) {
    case SPU_RdInMbox:
        if (s.mailbox_in_count > 0) {
            uint32_t v = s.mailbox_in[0];
            for (int i = 1; i < s.mailbox_in_count; ++i)
                s.mailbox_in[i - 1] = s.mailbox_in[i];
            --s.mailbox_in_count;
            return v;
        }
        return 0;
    case SPU_RdEventStat: return s.ch_event_stat;
    case SPU_RdEventMask: return s.ch_event_mask;
    case SPU_RdSigNotify1: return s.signal1;
    case SPU_RdSigNotify2: return s.signal2;
    case SPU_RdDec: return s.decrementer;
    default: return 0;
    }
}

// Execute one SPU instruction. Returns false if stopped.
static bool spu_step(SPUContext& s) {
    uint32_t pc = s.pc & 0x3FFFC;
    uint32_t insn = spu_ls_u32(s, pc);
    s.npc = (pc + 4) & 0x3FFFF;

    // stop / stopd: opcode 0x00000000 family with stop bit patterns
    // stop: 00000000 00IIIIIIIIIIIIIIIIIIIIII (RR)
    // Encoding: bits [31:21] == 0 for stop/lnop/nop variants in some docs;
    // Practical: 0x00000000 = stop, stopd has bit 0 of preferred
    if ((insn & 0xFFE00000u) == 0x00000000u) {
        // lnop 0x00200000, nop 0x40200000 handled below; pure stop
        if ((insn & 0x001FFFFFu) != 0 || insn == 0) {
            s.stop_status = (int)(insn & 0x3FFF);
            s.running = false;
            return false;
        }
    }

    unsigned op = (insn >> 21) & 0x7FF; // 11-bit for RR-ish
    unsigned rt = (insn >> 0) & 0x7F;
    unsigned ra = (insn >> 7) & 0x7F;
    unsigned rb = (insn >> 14) & 0x7F;
    unsigned rc = (insn >> 21) & 0x7F; // RRR uses different layout

    // RRR form: op in bits [31:28], rc/rb/ra/rt
    unsigned op4 = (insn >> 28) & 0xF;

    // Immediate forms
    int32_t i16 = (int32_t)(int16_t)(insn & 0xFFFF);
    int32_t i10 = (int32_t)((insn << 14) >> 22); // sign-extend 10-bit from bits [14:7]? vary by encoding
    // Cell uses several immediate placements; use common RI16: bits [25:7] style simplified

    auto finish = [&]() {
        s.pc = s.npc;
        return s.running;
    };

    // ---- Memory ----
    // lqd rt, i16(ra)   opcode 0x34 (pattern from ISA: 00110100...)
    // Use broad matching on top bits
    unsigned top7 = (insn >> 25) & 0x7F;
    unsigned top8 = (insn >> 24) & 0xFF;
    unsigned top9 = (insn >> 23) & 0x1FF;
    unsigned top10 = (insn >> 22) & 0x3FF;
    unsigned top11 = (insn >> 21) & 0x7FF;

    // lqd: 00110100 IIIIIIII IIIIIIII IRR RRRR R
    if (top8 == 0x34) {
        int32_t imm = (int32_t)(int16_t)((insn >> 7) & 0xFFFF);
        uint32_t addr = (spu_pref(s, ra) + (uint32_t)(imm << 4)) & 0x3FFFF;
        spu_ls_load16(s, rt, addr);
        return finish();
    }
    // stqd: 00100100
    if (top8 == 0x24) {
        int32_t imm = (int32_t)(int16_t)((insn >> 7) & 0xFFFF);
        uint32_t addr = (spu_pref(s, ra) + (uint32_t)(imm << 4)) & 0x3FFFF;
        spu_ls_store16(s, rt, addr);
        return finish();
    }
    // lqx: 00111000100
    if (top11 == 0x1C4) {
        uint32_t addr = (spu_pref(s, ra) + spu_pref(s, rb)) & 0x3FFFF;
        spu_ls_load16(s, rt, addr);
        return finish();
    }
    // stqx: 00101000100
    if (top11 == 0x144) {
        uint32_t addr = (spu_pref(s, ra) + spu_pref(s, rb)) & 0x3FFFF;
        spu_ls_store16(s, rt, addr);
        return finish();
    }
    // lqa: 00110001
    if (top8 == 0x31) {
        uint32_t addr = ((uint32_t)(int32_t)(int16_t)((insn >> 7) & 0xFFFF) << 2) & 0x3FFFF;
        spu_ls_load16(s, rt, addr);
        return finish();
    }
    // stqa: 00100001
    if (top8 == 0x21) {
        uint32_t addr = ((uint32_t)(int32_t)(int16_t)((insn >> 7) & 0xFFFF) << 2) & 0x3FFFF;
        spu_ls_store16(s, rt, addr);
        return finish();
    }

    // ---- Immediate load ----
    // il: 01000001
    if (top8 == 0x41) {
        int32_t imm = (int32_t)(int16_t)((insn >> 7) & 0xFFFF);
        spu_set_pref(s, rt, (uint32_t)imm);
        return finish();
    }
    // ilh: 0100001
    if (top7 == 0x21) {
        uint32_t imm = (insn >> 7) & 0xFFFF;
        spu_set_pref(s, rt, imm << 16);
        return finish();
    }
    // ilhu: 01000011
    if (top8 == 0x43) {
        uint32_t imm = (insn >> 7) & 0xFFFF;
        spu_set_pref(s, rt, imm << 16);
        return finish();
    }
    // iohl: 01000000
    if (top8 == 0x40) {
        uint32_t imm = (insn >> 7) & 0xFFFF;
        spu_set_pref(s, rt, spu_pref(s, rt) | imm);
        return finish();
    }

    // ---- Arithmetic RR ----
    // a: 00011000000
    if (top11 == 0x0C0) {
        spu_set_pref(s, rt, spu_pref(s, ra) + spu_pref(s, rb));
        return finish();
    }
    // ah: 00011001000
    if (top11 == 0x0C8) {
        spu_set_pref(s, rt, (spu_pref(s, ra) + spu_pref(s, rb)) & 0xFFFF);
        return finish();
    }
    // sf: 00001000000
    if (top11 == 0x040) {
        spu_set_pref(s, rt, spu_pref(s, rb) - spu_pref(s, ra));
        return finish();
    }
    // ai: 00011100
    if (top8 == 0x1C) {
        int32_t imm = (int32_t)((int32_t)(insn << 18) >> 22); // 10-bit
        spu_set_pref(s, rt, spu_pref(s, ra) + (uint32_t)imm);
        return finish();
    }
    // sfi: 00001100
    if (top8 == 0x0C) {
        int32_t imm = (int32_t)((int32_t)(insn << 18) >> 22);
        spu_set_pref(s, rt, (uint32_t)imm - spu_pref(s, ra));
        return finish();
    }
    // mpy: 01111000100
    if (top11 == 0x3C4) {
        int16_t a = (int16_t)(spu_pref(s, ra) & 0xFFFF);
        int16_t b = (int16_t)(spu_pref(s, rb) & 0xFFFF);
        spu_set_pref(s, rt, (uint32_t)((int32_t)a * (int32_t)b));
        return finish();
    }
    // mpyi: 01110100
    if (top8 == 0x74) {
        int16_t a = (int16_t)(spu_pref(s, ra) & 0xFFFF);
        int32_t imm = (int32_t)((int32_t)(insn << 18) >> 22);
        spu_set_pref(s, rt, (uint32_t)((int32_t)a * imm));
        return finish();
    }

    // ---- Logical ----
    // and: 00011000001
    if (top11 == 0x0C1) {
        spu_set_pref(s, rt, spu_pref(s, ra) & spu_pref(s, rb));
        return finish();
    }
    // or: 00001000001
    if (top11 == 0x041) {
        spu_set_pref(s, rt, spu_pref(s, ra) | spu_pref(s, rb));
        return finish();
    }
    // xor: 01001000001
    if (top11 == 0x241) {
        spu_set_pref(s, rt, spu_pref(s, ra) ^ spu_pref(s, rb));
        return finish();
    }
    // nand: 00011001001
    if (top11 == 0x0C9) {
        spu_set_pref(s, rt, ~(spu_pref(s, ra) & spu_pref(s, rb)));
        return finish();
    }
    // nor: 00001001001
    if (top11 == 0x049) {
        spu_set_pref(s, rt, ~(spu_pref(s, ra) | spu_pref(s, rb)));
        return finish();
    }
    // andi: 00010100
    if (top8 == 0x14) {
        int32_t imm = (int32_t)((int32_t)(insn << 18) >> 22);
        spu_set_pref(s, rt, spu_pref(s, ra) & (uint32_t)imm);
        return finish();
    }
    // ori: 00000100
    if (top8 == 0x04) {
        int32_t imm = (int32_t)((int32_t)(insn << 18) >> 22);
        spu_set_pref(s, rt, spu_pref(s, ra) | (uint32_t)imm);
        return finish();
    }
    // xori: 01000100
    if (top8 == 0x44) {
        int32_t imm = (int32_t)((int32_t)(insn << 18) >> 22);
        spu_set_pref(s, rt, spu_pref(s, ra) ^ (uint32_t)imm);
        return finish();
    }

    // ---- Shifts (scalar preferred slot) ----
    // shlqbi: 00111011011
    if (top11 == 0x1DB) {
        unsigned sh = spu_pref(s, rb) & 7;
        // byte shift of 128-bit - simplified on preferred word
        spu_set_pref(s, rt, spu_pref(s, ra) << sh);
        return finish();
    }
    // rotqbyi / similar - advance
    if (top11 == 0x1DC || top11 == 0x1D8) {
        spu_copy_reg(s, rt, ra);
        return finish();
    }

    // ---- Compare ----
    // ceq: 01111000000
    if (top11 == 0x3C0) {
        spu_set_pref(s, rt, spu_pref(s, ra) == spu_pref(s, rb) ? 0xFFFFFFFFu : 0);
        return finish();
    }
    // cgt: 01001000000
    if (top11 == 0x240) {
        spu_set_pref(s, rt, (int32_t)spu_pref(s, ra) > (int32_t)spu_pref(s, rb) ? 0xFFFFFFFFu : 0);
        return finish();
    }
    // clgt: 01011000000
    if (top11 == 0x2C0) {
        spu_set_pref(s, rt, spu_pref(s, ra) > spu_pref(s, rb) ? 0xFFFFFFFFu : 0);
        return finish();
    }
    // ceqi: 01111100
    if (top8 == 0x7C) {
        int32_t imm = (int32_t)((int32_t)(insn << 18) >> 22);
        spu_set_pref(s, rt, (int32_t)spu_pref(s, ra) == imm ? 0xFFFFFFFFu : 0);
        return finish();
    }

    // ---- Select bits (selb) RRR: 1 0 0 0 ----
    if (op4 == 0x8) {
        // bits: 1000 rc rb ra rt
        unsigned rrc = (insn >> 21) & 0x7F;
        unsigned rrb = (insn >> 14) & 0x7F;
        unsigned rra = (insn >> 7) & 0x7F;
        unsigned rrt = insn & 0x7F;
        for (int i = 0; i < 16; ++i) {
            uint8_t mask = s.gpr[rrc][i];
            s.gpr[rrt][i] = (uint8_t)((s.gpr[rrb][i] & mask) | (s.gpr[rra][i] & ~mask));
        }
        return finish();
    }

    // ---- Branches ----
    // br: 001100100
    if (top9 == 0x64) {
        int32_t off = (int32_t)((int32_t)(insn << 7) >> 7); // 16-bit I16 << 2 style
        off = (int32_t)(int16_t)(insn & 0xFFFF);
        s.npc = (pc + (uint32_t)(off << 2)) & 0x3FFFF;
        return finish();
    }
    // bra: 001100000
    if (top9 == 0x60) {
        int32_t off = (int32_t)(int16_t)(insn & 0xFFFF);
        s.npc = ((uint32_t)(off << 2)) & 0x3FFFF;
        return finish();
    }
    // brz: 001000100
    if (top9 == 0x44) {
        int32_t off = (int32_t)(int16_t)((insn >> 7) & 0xFFFF);
        if (spu_pref(s, rt) == 0)
            s.npc = (pc + (uint32_t)(off << 2)) & 0x3FFFF;
        return finish();
    }
    // brnz: 001000000
    if (top9 == 0x40) {
        int32_t off = (int32_t)(int16_t)((insn >> 7) & 0xFFFF);
        if (spu_pref(s, rt) != 0)
            s.npc = (pc + (uint32_t)(off << 2)) & 0x3FFFF;
        return finish();
    }
    // brasl / brsl: set LR (reg 0 is link on SPU - actually reg 0 often used)
    if (top9 == 0x66 || top9 == 0x62) {
        int32_t off = (int32_t)(int16_t)(insn & 0xFFFF);
        spu_set_pref(s, rt, s.npc);
        if (top9 == 0x66)
            s.npc = (pc + (uint32_t)(off << 2)) & 0x3FFFF;
        else
            s.npc = ((uint32_t)(off << 2)) & 0x3FFFF;
        return finish();
    }
    // bi: 00110101000
    if (top11 == 0x1A8) {
        s.npc = spu_pref(s, ra) & 0x3FFFC;
        return finish();
    }
    // bisl: 00110101001
    if (top11 == 0x1A9) {
        spu_set_pref(s, rt, s.npc);
        s.npc = spu_pref(s, ra) & 0x3FFFC;
        return finish();
    }
    // biz / binz
    if (top11 == 0x1AA || top11 == 0x1AB) {
        bool take = (top11 == 0x1AA) ? (spu_pref(s, rt) == 0) : (spu_pref(s, rt) != 0);
        if (take) s.npc = spu_pref(s, ra) & 0x3FFFC;
        return finish();
    }

    // ---- Channels ----
    // wrch: 00100000001
    if (top11 == 0x101) {
        unsigned ch = ra; // channel in ra field for wrch rt,ca
        // actual encoding: wrch ca,rt  - ca in bits
        ch = (insn >> 7) & 0x7F;
        spu_channel_write(s, ch, spu_pref(s, rt));
        return finish();
    }
    // rdch: 00000000001
    if (top11 == 0x001) {
        unsigned ch = (insn >> 7) & 0x7F;
        spu_set_pref(s, rt, spu_channel_read(s, ch));
        return finish();
    }

    // lnop / nop
    if (insn == 0x00200000u || insn == 0x40200000u) {
        return finish();
    }

    // stopd: 00101000000
    if (top11 == 0x140) {
        s.stop_status = 0;
        s.running = false;
        return false;
    }

    // Unknown: treat as nop but count
    static int unk = 0;
    if (unk < 16) {
        std::fprintf(stderr, "[spu] unk insn 0x%08x at pc=0x%x\n", insn, pc);
        ++unk;
    }
    return finish();
}

static void spu_run_interpreter(SPUContext& s, int max_steps) {
    for (int i = 0; i < max_steps && s.running; ++i) {
        if (!spu_step(s)) break;
    }
    s.running = false;
}

} // namespace

PS3RT_API void ps3rt_touch(uint64_t addr, size_t len) {
    if (!g_mem || len == 0) return;
    if (addr >= kGuestSize) return;
    map_region(addr, len);
}

PS3RT_API int ps3rt_init(const char* image_path) {
    FILE* f = std::fopen(image_path, "rb");
    if (!f) return 1;
    char magic[8]; uint32_t nseg = 0;
    if (std::fread(magic, 1, 8, f) != 8 || std::memcmp(magic, "PS3IMG1\0", 8) != 0 || std::fread(&nseg, 4, 1, f) != 1) {
        std::fclose(f); return 2;
    }
#if defined(_WIN32)
    g_mem = (uint8_t*)VirtualAlloc(nullptr, (SIZE_T)kGuestSize, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* p = mmap(nullptr, (size_t)kGuestSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    g_mem = (p == MAP_FAILED) ? nullptr : (uint8_t*)p;
#endif
    if (!g_mem) { std::fclose(f); return 3; }
    struct Seg { uint64_t vaddr, filesz, memsz, off; };
    std::vector<Seg> segs(nseg);
    if (nseg && std::fread(segs.data(), sizeof(Seg), nseg, f) != nseg) { std::fclose(f); return 2; }
    for (const Seg& s : segs) {
        if (s.vaddr >= kGuestSize) continue;
        uint64_t memsz = s.memsz;
        if (s.vaddr + memsz > kGuestSize) memsz = kGuestSize - s.vaddr;
        map_region(s.vaddr, (memsz + 0xFFFF) & ~0xFFFFull);
        std::fseek(f, (long)s.off, SEEK_SET);
        uint64_t filesz = s.filesz < memsz ? s.filesz : memsz;
        if (filesz && std::fread(g_mem + s.vaddr, 1, (size_t)filesz, f) != filesz) { std::fclose(f); return 2; }
    }
    std::fclose(f);
    map_region(kStackBase, kStackSize);
    map_region(kHeapBase, kHeapSize);
    g_heap_next = kHeapBase;
    std::memset(g_spu_used, 0, sizeof g_spu_used);
    std::fprintf(stderr, "[ps3rt] image loaded, segments=%u stack=0x%llx heap=0x%llx\n",
        nseg, (unsigned long long)kStackBase, (unsigned long long)kHeapBase);
    return 0;
}

PS3RT_API uint8_t* ps3rt_memory(void)    { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize - 0x100; }
PS3RT_API void ps3rt_shutdown(void) {
#if defined(_WIN32)
    if (g_mem) VirtualFree(g_mem, 0, MEM_RELEASE);
#else
    if (g_mem) munmap(g_mem, (size_t)kGuestSize);
#endif
    g_mem = nullptr;
}

PS3RT_API int ps3rt_spu_supported(void) { return 1; }

PS3RT_API int ps3rt_spu_create(int* out_id) {
    std::lock_guard<std::mutex> lock(g_spu_mu);
    for (int i = 0; i < kMaxSpu; ++i) {
        if (!g_spu_used[i]) {
            g_spu_used[i] = true;
            std::memset(&g_spu[i], 0, sizeof(SPUContext));
            if (out_id) *out_id = i;
            std::fprintf(stderr, "[spu] create id=%d\n", i);
            return 0;
        }
    }
    return -1;
}

PS3RT_API int ps3rt_spu_destroy(int id) {
    if (id < 0 || id >= kMaxSpu) return -1;
    std::lock_guard<std::mutex> lock(g_spu_mu);
    g_spu_used[id] = false;
    g_spu[id].running = false;
    return 0;
}

PS3RT_API int ps3rt_spu_load(int id, const void* img, size_t len) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !img) return -1;
    if (len > sizeof(g_spu[id].ls)) len = sizeof(g_spu[id].ls);
    std::memset(g_spu[id].ls, 0, sizeof(g_spu[id].ls));
    std::memcpy(g_spu[id].ls, img, len);
    std::fprintf(stderr, "[spu] load id=%d bytes=%zu\n", id, len);
    return 0;
}

PS3RT_API int ps3rt_spu_run(int id, uint32_t entry) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu[id].pc = entry & ~3u;
    g_spu[id].running = true;
    g_spu[id].stop_status = 0;
    // Up to 2M steps per run invocation
    spu_run_interpreter(g_spu[id], 2 * 1024 * 1024);
    std::fprintf(stderr, "[spu] run id=%d entry=0x%x stop=%d pc=0x%x\n",
        id, entry, g_spu[id].stop_status, g_spu[id].pc);
    return g_spu[id].stop_status;
}

PS3RT_API int ps3rt_spu_stop(int id) {
    if (id < 0 || id >= kMaxSpu) return -1;
    g_spu[id].running = false;
    return 0;
}

PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    if (g_spu[id].mailbox_in_count >= 4) return -1;
    g_spu[id].mailbox_in[g_spu[id].mailbox_in_count++] = val;
    return 0;
}

PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !out) return -1;
    if (g_spu[id].mailbox_out_count <= 0) { *out = 0; return -1; }
    *out = g_spu[id].mailbox_out[0];
    for (int i = 1; i < g_spu[id].mailbox_out_count; ++i)
        g_spu[id].mailbox_out[i - 1] = g_spu[id].mailbox_out[i];
    --g_spu[id].mailbox_out_count;
    return 0;
}

PS3RT_API int ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !g_mem) return -1;
    lsa &= 0x3FFFF;
    if ((uint64_t)lsa + size > sizeof(g_spu[id].ls)) return -1;
    if (ea >= kGuestSize) return -1;
    map_region(ea, size);
    if (cmd & 0x40)
        std::memcpy(g_mem + ea, g_spu[id].ls + lsa, size);
    else
        std::memcpy(g_spu[id].ls + lsa, g_mem + ea, size);
    return 0;
}

PS3RT_API void ps3rt_syscall(PPUContext* c) {
    uint64_t n = c->gpr[11];
    switch (n) {
    case 1:
    case 22:
    case 41:
        c->halted = true; break;

    case 348:
    case 352: {
        uint64_t size = c->gpr[3];
        if (size == 0) size = 0x10000;
        size = (size + 0xFFFF) & ~0xFFFFull;
        if (g_heap_next + size > kHeapBase + kHeapSize) {
            c->gpr[3] = 0x80010005ull;
            break;
        }
        uint64_t addr = g_heap_next;
        g_heap_next += size;
        map_region(addr, size);
        if (c->gpr[5]) wr64(*c, c->gpr[5], addr);
        c->gpr[3] = 0;
        break;
    }

    case 170: case 171: case 172: case 173:
    case 182: case 183: case 185: case 190: case 191: case 193:
    {
        int sid = -1;
        if (n == 182 || n == 170) {
            ps3rt_spu_create(&sid);
            if (c->gpr[3] && sid >= 0) wr32(*c, c->gpr[3], (uint32_t)sid);
        }
        if (n == 172 && sid < 0) {
            // start: run first used SPU if any
            for (int i = 0; i < kMaxSpu; ++i)
                if (g_spu_used[i]) { ps3rt_spu_run(i, g_spu[i].pc); break; }
        }
        c->gpr[3] = 0;
        break;
    }

    case 403: {
        uint64_t buf = c->gpr[4], len = c->gpr[5];
        if (buf < kGuestSize && len && g_mem) {
            map_region(buf, (size_t)len);
            std::fwrite(g_mem + buf, 1, (size_t)len, stdout);
            std::fflush(stdout);
        }
        if (c->gpr[6]) wr32(*c, c->gpr[6], (uint32_t)len);
        c->gpr[3] = 0; break;
    }

    default:
        c->gpr[3] = 0;
        break;
    }
}

PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int count = 0;
    if (count < 20) {
        std::fprintf(stderr,
            "[ps3rt] unimplemented insn 0x%08x at pc=0x%llx\n"
            "  lr=0x%llx ctr=0x%llx r1=0x%llx r2=0x%llx r3=0x%llx\n",
            opcode, (unsigned long long)pc,
            (unsigned long long)c->lr, (unsigned long long)c->ctr,
            (unsigned long long)c->gpr[1], (unsigned long long)c->gpr[2],
            (unsigned long long)c->gpr[3]);
        ++count;
    }
}
