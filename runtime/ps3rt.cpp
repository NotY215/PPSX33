// ps3rt - see prior; LV2 expanded
// User: pull project.cpp first (rewritten with nid_table.h + Compilers-files)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "nid_table.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <algorithm>

static uint8_t* g_mem = nullptr;
static size_t g_mem_size = 0;
static const uint64_t kStackBase = 0x10000000ull;
static const size_t kStackSize = 0x100000;
static std::mutex g_lock;

PS3RT_API void ps3rt_touch(uint64_t, size_t) {}
PS3RT_API int ps3rt_init(const char* image_path) {
    std::lock_guard<std::mutex> lk(g_lock);
    if (g_mem) return 0;
    g_mem_size = 256ull * 1024 * 1024;
    g_mem = (uint8_t*)std::calloc(1, g_mem_size);
    if (!g_mem) return -1;
    const char* candidates[] = { image_path, "guest_image.bin", "output/guest_image.bin" };
    for (const char* p : candidates) {
        if (!p) continue;
        FILE* f = std::fopen(p, "rb");
        if (!f) continue;
        std::fseek(f, 0, SEEK_END); long sz = std::ftell(f); std::fseek(f, 0, SEEK_SET);
        if (sz > 0 && (size_t)sz <= g_mem_size) {
            size_t n = std::fread(g_mem, 1, (size_t)sz, f);
            std::fprintf(stderr, "[ps3rt] loaded %zu bytes from %s\n", n, p);
        }
        std::fclose(f); break;
    }
    return 0;
}
PS3RT_API uint8_t* ps3rt_memory(void) { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize - 0x100; }
PS3RT_API void ps3rt_shutdown(void) {
    std::lock_guard<std::mutex> lk(g_lock);
    std::free(g_mem); g_mem = nullptr; g_mem_size = 0;
}

static constexpr int kMaxSpu = 6;
static SPUContext g_spu[kMaxSpu];
static bool g_spu_used[kMaxSpu] = {};
static inline uint32_t spu_pref(const SPUContext& s, unsigned r) { uint32_t v; std::memcpy(&v, s.gpr[r], 4); return v; }
static inline void spu_set_pref(SPUContext& s, unsigned r, uint32_t v) {
    std::memcpy(s.gpr[r], &v, 4);
    for (int i = 1; i < 4; ++i) std::memcpy(s.gpr[r] + i*4, &v, 4);
}
static inline void spu_ls_load16(SPUContext& s, unsigned rt, uint32_t addr) { addr &= 0x3FFF0; std::memcpy(s.gpr[rt], s.ls + addr, 16); }
static inline void spu_ls_store16(SPUContext& s, unsigned rt, uint32_t addr) { addr &= 0x3FFF0; std::memcpy(s.ls + addr, s.gpr[rt], 16); }

static void spu_channel_write(SPUContext& s, unsigned ch, uint32_t val) {
    switch (ch) {
    case 1: s.ch_event_mask = val; break;
    case 7: s.decrementer = val; break;
    case 28: case 29: if (s.mailbox_out_count < 4) s.mailbox_out[s.mailbox_out_count++] = val; break;
    case 16: s.mfc_lsa = val & 0x3FFFF; break;
    case 17: s.mfc_ea = (s.mfc_ea & 0xFFFFFFFFull) | ((uint64_t)val << 32); break;
    case 18: s.mfc_ea = (s.mfc_ea & 0xFFFFFFFF00000000ull) | val; break;
    case 19: s.mfc_size = val & 0x7FFF; break;
    case 20: s.mfc_tag = val & 0x1F; break;
    case 21:
        s.mfc_cmd = val;
        if (g_mem && s.mfc_size) {
            uint32_t lsa = s.mfc_lsa & 0x3FFFF; uint64_t ea = s.mfc_ea; uint32_t sz = s.mfc_size;
            if (lsa + sz <= 256*1024 && ea + sz <= g_mem_size) {
                if ((val & 0x1F) == 0x40 || (val & 0x1F) == 0x41) std::memcpy(s.ls + lsa, g_mem + ea, sz);
                else std::memcpy(g_mem + ea, s.ls + lsa, sz);
            }
        }
        break;
    default: break;
    }
}
static uint32_t spu_channel_read(SPUContext& s, unsigned ch) {
    switch (ch) {
    case 0: return s.ch_event_stat;
    case 1: return s.ch_event_mask;
    case 7: return s.decrementer;
    case 29:
        if (s.mailbox_in_count > 0) {
            uint32_t v = s.mailbox_in[0];
            for (int i = 1; i < s.mailbox_in_count; ++i) s.mailbox_in[i-1] = s.mailbox_in[i];
            --s.mailbox_in_count; return v;
        }
        return 0;
    case 24: return (uint32_t)s.mailbox_in_count;
    case 28: return 4u - (uint32_t)s.mailbox_out_count;
    default: return 0;
    }
}

static bool spu_step(SPUContext& s) {
    uint32_t pc = s.pc & 0x3FFFC; uint32_t op;
    std::memcpy(&op, s.ls + pc, 4);
    op = (op >> 24) | ((op >> 8) & 0xFF00) | ((op << 8) & 0xFF0000) | (op << 24);
    unsigned rt = (op >> 21) & 0x7F, ra = (op >> 14) & 0x7F, rb = (op >> 7) & 0x7F, major = op >> 21;
    if ((op >> 21) == 0x000 || (op >> 21) == 0x001) { s.stop_status = (int)(op & 0x3FFF); s.running = false; return false; }
    auto binary_u32 = [&](uint32_t (*fn)(uint32_t, uint32_t)) { spu_set_pref(s, rt, fn(spu_pref(s, ra), spu_pref(s, rb))); };
    switch (major) {
    case 0x32: case 0x30: { int32_t i16 = (int32_t)(int16_t)(op & 0xFFFF); s.pc = (pc + (i16 << 2)) & 0x3FFFF; return true; }
    case 0x31: { s.pc = ((op & 0xFFFF) << 2) & 0x3FFFF; return true; }
    case 0x33: { int32_t i16 = (int32_t)(int16_t)(op & 0xFFFF); spu_set_pref(s, 0, pc + 4); s.pc = (pc + (i16 << 2)) & 0x3FFFF; return true; }
    case 0x28: { s.pc = spu_pref(s, ra) & 0x3FFFC; return true; }
    case 0x34: { int32_t i10 = (int32_t)((op << 22) >> 22); spu_ls_load16(s, rt, (spu_pref(s, ra) + (i10 << 4)) & 0x3FFFF); break; }
    case 0x24: { int32_t i10 = (int32_t)((op << 22) >> 22); spu_ls_store16(s, rt, (spu_pref(s, ra) + (i10 << 4)) & 0x3FFFF); break; }
    case 0x1C: spu_ls_load16(s, rt, (spu_pref(s, ra) + spu_pref(s, rb)) & 0x3FFFF); break;
    case 0x1D: spu_ls_store16(s, rt, (spu_pref(s, ra) + spu_pref(s, rb)) & 0x3FFFF); break;
    case 0x04: case 0x05: { uint32_t imm = op & 0xFFFF; if (major == 0x05) imm <<= 16; spu_set_pref(s, rt, spu_pref(s, ra) | imm); break; }
    case 0x0C: case 0x0D: { uint32_t imm = op & 0xFFFF; if (major == 0x0D) imm <<= 16; spu_set_pref(s, rt, spu_pref(s, ra) & imm); break; }
    case 0x14: case 0x15: { uint32_t imm = op & 0xFFFF; if (major == 0x15) imm <<= 16; spu_set_pref(s, rt, spu_pref(s, ra) ^ imm); break; }
    case 0x08: case 0x09: case 0x21: spu_set_pref(s, rt, op & 0xFFFF); break;
    case 0x0A: binary_u32([](uint32_t a, uint32_t b){ return a + b; }); break;
    case 0x0B: binary_u32([](uint32_t a, uint32_t b){ return b - a; }); break;
    case 0x0E: binary_u32([](uint32_t a, uint32_t b){ return a & b; }); break;
    case 0x0F: binary_u32([](uint32_t a, uint32_t b){ return a | b; }); break;
    case 0x16: binary_u32([](uint32_t a, uint32_t b){ return a ^ b; }); break;
    case 0x25: spu_channel_write(s, (op >> 7) & 0x7F, spu_pref(s, rt)); break;
    default:
        if ((op & 0xFFE00000u) == 0x01A00000u) spu_set_pref(s, rt, spu_channel_read(s, (op >> 7) & 0x7F));
        break;
    }
    s.pc = (pc + 4) & 0x3FFFF; return true;
}
static void spu_run_interpreter(SPUContext& s, int max_steps) {
    for (int i = 0; i < max_steps && s.running; ++i) { if (!spu_step(s)) break; if (s.decrementer) --s.decrementer; }
}

PS3RT_API int ps3rt_spu_supported(void) { return 1; }
PS3RT_API int ps3rt_spu_create(int* out_id) {
    for (int i = 0; i < kMaxSpu; ++i) if (!g_spu_used[i]) { g_spu_used[i] = true; std::memset(&g_spu[i], 0, sizeof(SPUContext)); if (out_id) *out_id = i; return 0; }
    return -1;
}
PS3RT_API int ps3rt_spu_destroy(int id) { if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1; g_spu_used[id] = false; return 0; }
PS3RT_API int ps3rt_spu_load(int id, const void* img, size_t len) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !img) return -1;
    std::memcpy(g_spu[id].ls, img, std::min(len, sizeof(g_spu[id].ls))); return 0;
}
PS3RT_API int ps3rt_spu_run(int id, uint32_t entry) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu[id].pc = entry & ~3u; g_spu[id].running = true; spu_run_interpreter(g_spu[id], 1 << 20); return 0;
}
PS3RT_API int ps3rt_spu_stop(int id) { if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1; g_spu[id].running = false; return 0; }
PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    if (g_spu[id].mailbox_in_count >= 4) return -1;
    g_spu[id].mailbox_in[g_spu[id].mailbox_in_count++] = val; return 0;
}
PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !out) return -1;
    if (g_spu[id].mailbox_out_count <= 0) return -1;
    *out = g_spu[id].mailbox_out[0];
    for (int i = 1; i < g_spu[id].mailbox_out_count; ++i) g_spu[id].mailbox_out[i-1] = g_spu[id].mailbox_out[i];
    --g_spu[id].mailbox_out_count; return 0;
}
PS3RT_API int ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !g_mem) return -1;
    lsa &= 0x3FFFF;
    if (lsa + size > sizeof(g_spu[id].ls) || ea + size > g_mem_size) return -1;
    if ((cmd & 0x1F) == 0x40 || (cmd & 0x1F) == 0x41) std::memcpy(g_spu[id].ls + lsa, g_mem + ea, size);
    else std::memcpy(g_mem + ea, g_spu[id].ls + lsa, size);
    return 0;
}

// Expanded LV2 HLE — return CELL_OK for common boot paths
PS3RT_API void ps3rt_syscall(PPUContext* c) {
    if (!c) return;
    uint64_t num = c->gpr[11];
    switch (num) {
    case 1: c->halted = true; break;
    case 3:
        if ((c->gpr[3] == 1 || c->gpr[3] == 2) && g_mem) {
            uint64_t buf = c->gpr[4], len = c->gpr[5];
            if (buf + len <= g_mem_size) std::fwrite(g_mem + buf, 1, (size_t)len, stderr);
            c->gpr[3] = len;
        } else c->gpr[3] = 0;
        break;
    case 4: case 18: case 19: case 20: case 22: case 25:
    case 41: case 43: case 44: case 48: case 53:
    case 70: case 73:
    case 90: case 91: case 93: case 94:
    case 100: case 101: case 102: case 103: case 104:
    case 105: case 106: case 107: case 108: case 109:
    case 114: case 115: case 116: case 117: case 118:
    case 120: case 121: case 122: case 123: case 124:
    case 128: case 129: case 130: case 131: case 132:
    case 141: case 142:
    case 348: case 349: case 352: case 353: case 403:
    case 801: case 802: case 803: case 804: case 808: case 809: case 811: case 812: case 814:
        c->gpr[3] = 0; break;
    default: c->gpr[3] = 0; break;
    }
}

static int g_prx_log = 0;
static int prx_dispatch(PPUContext* c, int kind, const char* name) {
    uint32_t maybe_nid = (uint32_t)c->gpr[11];
    const NidEntry* e = nid_lookup(maybe_nid);
    if (e) {
        if (g_prx_log < 32) { std::fprintf(stderr, "[prx] %s::%s nid=0x%08X\n", e->module, e->name, e->nid); ++g_prx_log; }
        kind = e->kind; name = e->name;
    } else if (g_prx_log < 16) {
        std::fprintf(stderr, "[prx] stub kind=%d %s pc=0x%llx\n", kind, name ? name : "?", (unsigned long long)c->pc);
        ++g_prx_log;
    }
    c->gpr[3] = 0; return 0;
}
PS3RT_API int ps3rt_prx_import_stub(PPUContext* c, uint64_t pc) {
    if (!c) return -1;
    if (pc == 0x39800000ull || (pc & 0xFFFF0000ull) == 0x39800000ull) { prx_dispatch(c, 1, "cellGcmSys"); return 0; }
    prx_dispatch(c, prx_module_kind("unknown"), "import"); return 0;
}
PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int n = 0;
    if (n < 8) { std::fprintf(stderr, "[ps3] unimplemented op=0x%08X pc=0x%llx\n", opcode, (unsigned long long)pc); ++n; }
    if (c) c->halted = true;
}
