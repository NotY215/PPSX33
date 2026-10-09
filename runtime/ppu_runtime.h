// ppu_runtime.h - portable MSVC + GCC/Clang
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <cstdio>

#if defined(_MSC_VER)
  #include <stdlib.h>
  #include <intrin.h>
  #ifdef PS3RT_BUILD_DLL
    #define PS3RT_API extern "C" __declspec(dllexport)
  #else
    #define PS3RT_API extern "C" __declspec(dllimport)
  #endif
#else
  #define PS3RT_API extern "C" __attribute__((visibility("default")))
#endif

struct PPUContext {
    uint64_t gpr[32];
    double   fpr[32];
    uint8_t  vpr[32][16];
    uint64_t lr, ctr, xer, pc;
    uint8_t  cr[8];
    uint32_t fpscr;
    uint32_t vscr;
    uint8_t* mem;
    bool     halted;
    int      thread_id;
    uint64_t res_addr;
    int      res_size;
};

// Lightweight SPU context (one per logical SPU thread)
struct SPUContext {
    uint32_t gpr[128];       // 128 x 128-bit would be heavy; use 32-bit slots for stubs
    uint8_t  ls[256 * 1024]; // 256 KB local store
    uint32_t pc;
    uint32_t npc;
    bool     running;
    bool     isolated;
    uint32_t mailbox_in[4];
    int      mailbox_in_count;
    uint32_t mailbox_out[4];
    int      mailbox_out_count;
    uint32_t signal1, signal2;
    uint64_t mfc_ea;
    uint32_t mfc_lsa;
    uint32_t mfc_size;
    uint32_t mfc_tag;
    uint32_t mfc_cmd;
};

static inline uint16_t bs16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }
static inline uint32_t bs32(uint32_t v) {
#if defined(_MSC_VER)
    return _byteswap_ulong(v);
#else
    return __builtin_bswap32(v);
#endif
}
static inline uint64_t bs64(uint64_t v) {
#if defined(_MSC_VER)
    return _byteswap_uint64(v);
#else
    return __builtin_bswap64(v);
#endif
}

static inline uint32_t ps3_clz32(uint32_t v) {
    if (v == 0) return 32;
#if defined(_MSC_VER)
    unsigned long i; _BitScanReverse(&i, v); return 31u - (uint32_t)i;
#else
    return (uint32_t)__builtin_clz(v);
#endif
}
static inline uint64_t ps3_clz64(uint64_t v) {
    if (v == 0) return 64;
#if defined(_MSC_VER)
    unsigned long i; _BitScanReverse64(&i, v); return 63u - (uint64_t)i;
#else
    return (uint64_t)__builtin_clzll(v);
#endif
}

static inline uint64_t ps3_rotl64(uint64_t v, unsigned s) {
    s &= 63u; if (s == 0) return v; return (v << s) | (v >> (64u - s));
}
static inline uint64_t ps3_rotl32dup(uint32_t v, unsigned s) {
    uint64_t x = ((uint64_t)v << 32) | (uint64_t)v;
    return ps3_rotl64(x, s & 31u);
}

// Demand-commit helper (implemented in ps3rt.cpp)
PS3RT_API void ps3rt_touch(uint64_t addr, size_t len);

static inline void touch_rw(PPUContext& c, uint64_t a, size_t n) {
    (void)c;
    ps3rt_touch(a, n);
}

static inline uint8_t  rd8 (PPUContext& c, uint64_t a){ touch_rw(c, a, 1); return c.mem[a]; }
static inline uint16_t rd16(PPUContext& c, uint64_t a){ touch_rw(c, a, 2); uint16_t v; std::memcpy(&v, c.mem + a, 2); return bs16(v); }
static inline uint32_t rd32(PPUContext& c, uint64_t a){ touch_rw(c, a, 4); uint32_t v; std::memcpy(&v, c.mem + a, 4); return bs32(v); }
static inline uint64_t rd64(PPUContext& c, uint64_t a){ touch_rw(c, a, 8); uint64_t v; std::memcpy(&v, c.mem + a, 8); return bs64(v); }
static inline void wr8 (PPUContext& c, uint64_t a, uint8_t  v){ touch_rw(c, a, 1); c.mem[a] = v; }
static inline void wr16(PPUContext& c, uint64_t a, uint16_t v){ touch_rw(c, a, 2); v = bs16(v); std::memcpy(c.mem + a, &v, 2); }
static inline void wr32(PPUContext& c, uint64_t a, uint32_t v){ touch_rw(c, a, 4); v = bs32(v); std::memcpy(c.mem + a, &v, 4); }
static inline void wr64(PPUContext& c, uint64_t a, uint64_t v){ touch_rw(c, a, 8); v = bs64(v); std::memcpy(c.mem + a, &v, 8); }

static inline void vpr_load(PPUContext& c, unsigned v, uint64_t addr) {
    addr &= ~0xFull;
    touch_rw(c, addr, 16);
    std::memcpy(c.vpr[v], c.mem + addr, 16);
}
static inline void vpr_store(PPUContext& c, unsigned v, uint64_t addr) {
    addr &= ~0xFull;
    touch_rw(c, addr, 16);
    std::memcpy(c.mem + addr, c.vpr[v], 16);
}
static inline void vpr_and(PPUContext& c, unsigned vd, unsigned va, unsigned vb) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = c.vpr[va][i] & c.vpr[vb][i];
}
static inline void vpr_or(PPUContext& c, unsigned vd, unsigned va, unsigned vb) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = c.vpr[va][i] | c.vpr[vb][i];
}
static inline void vpr_xor(PPUContext& c, unsigned vd, unsigned va, unsigned vb) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = c.vpr[va][i] ^ c.vpr[vb][i];
}
static inline void vpr_splat_u8(PPUContext& c, unsigned vd, uint8_t x) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = x;
}
static inline void vpr_splat_u32(PPUContext& c, unsigned vd, uint32_t x) {
    for (int i = 0; i < 4; ++i) {
        uint32_t be = bs32(x);
        std::memcpy(&c.vpr[vd][i * 4], &be, 4);
    }
}

static inline float rd_f32(PPUContext& c, uint64_t a) {
    uint32_t bits = rd32(c, a); float f; std::memcpy(&f, &bits, 4); return f;
}
static inline double rd_f64(PPUContext& c, uint64_t a) {
    uint64_t bits = rd64(c, a); double d; std::memcpy(&d, &bits, 8); return d;
}
static inline void wr_f32(PPUContext& c, uint64_t a, float f) {
    uint32_t bits; std::memcpy(&bits, &f, 4); wr32(c, a, bits);
}
static inline void wr_f64(PPUContext& c, uint64_t a, double d) {
    uint64_t bits; std::memcpy(&bits, &d, 8); wr64(c, a, bits);
}

static inline void set_cr_signed(PPUContext& c, int f, int64_t a, int64_t b){
    c.cr[f] = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
}
static inline void set_cr_unsigned(PPUContext& c, int f, uint64_t a, uint64_t b){
    c.cr[f] = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
}
static inline void set_cr_fp(PPUContext& c, int f, double a, double b){
    if (std::isnan(a) || std::isnan(b)) c.cr[f] = 1;
    else c.cr[f] = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
}

static inline bool bc_taken(PPUContext& c, unsigned bo, unsigned bi){
    bool ctr_ok = true, cond_ok = true;
    if (!(bo & 0x04)) { c.ctr -= 1; ctr_ok = ((c.ctr != 0) != ((bo & 0x02) != 0)); }
    if (!(bo & 0x10)) {
        bool bit = (c.cr[bi >> 2] >> (3 - (bi & 3))) & 1;
        cond_ok = (bit == ((bo & 0x08) != 0));
    }
    return ctr_ok && cond_ok;
}

PS3RT_API void ps3rt_syscall(PPUContext* c);
PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc);
PS3RT_API int      ps3rt_init(const char* image_path);
PS3RT_API uint8_t* ps3rt_memory(void);
PS3RT_API uint64_t ps3rt_stack_top(void);
PS3RT_API void     ps3rt_shutdown(void);

// SPU API
PS3RT_API int  ps3rt_spu_supported(void);
PS3RT_API int  ps3rt_spu_create(int* out_id);
PS3RT_API int  ps3rt_spu_destroy(int id);
PS3RT_API int  ps3rt_spu_load(int id, const void* img, size_t len);
PS3RT_API int  ps3rt_spu_run(int id, uint32_t entry);
PS3RT_API int  ps3rt_spu_stop(int id);
PS3RT_API int  ps3rt_spu_mbox_write(int id, uint32_t val);
PS3RT_API int  ps3rt_spu_mbox_read(int id, uint32_t* out);
PS3RT_API int  ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd);

struct PPUChunk { uint64_t start, end; bool (*fn)(PPUContext&); };

// When PC leaves recompiled code, try return-via-lr (treat as external stub).
// Caps repeated escapes so we still halt on infinite external bounce.
static inline void ppu_run(PPUContext& c, const PPUChunk* chunks, size_t n){
    int external_escapes = 0;
    while (!c.halted) {
        const PPUChunk* hit = nullptr;
        for (size_t i = 0; i < n; ++i)
            if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { hit = &chunks[i]; break; }
        if (!hit) {
            std::fprintf(stderr,
                "[ps3] PC left recompiled range\n"
                "  pc = 0x%llx\n  lr = 0x%llx\n  ctr = 0x%llx\n  r1 = 0x%llx\n  r2 = 0x%llx\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr,
                (unsigned long long)c.ctr, (unsigned long long)c.gpr[1],
                (unsigned long long)c.gpr[2]);
            // External / unresolved target: return via LR if it lands back in code
            if (c.lr && c.lr != c.pc) {
                bool lr_ok = false;
                for (size_t i = 0; i < n; ++i)
                    if (c.lr >= chunks[i].start && c.lr < chunks[i].end) { lr_ok = true; break; }
                if (lr_ok && external_escapes < 64) {
                    std::fprintf(stderr, "[ps3] external stub: return via lr=0x%llx (escape %d)\n",
                        (unsigned long long)c.lr, external_escapes + 1);
                    c.gpr[3] = 0; // CELL_OK / success for most lib stubs
                    c.pc = c.lr;
                    ++external_escapes;
                    continue;
                }
            }
            c.halted = true;
            break;
        }
        external_escapes = 0;
        hit->fn(c);
    }
}
