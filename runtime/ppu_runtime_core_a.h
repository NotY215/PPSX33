// ppu_runtime_core_a.h - portable MSVC + GCC/Clang
// XER/FPSCR/VMX helpers + bounds-checked guest memory access
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdarg>

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

struct SPUContext {
    uint8_t  gpr[128][16];   // 128 x 128-bit (16 bytes) GPRs
    uint8_t  ls[256 * 1024]; // 256 KiB local store
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
    uint32_t mfc_tag_mask;     // XX-K/L: tag wait mask
    uint32_t mfc_tag_status;   // completed tags bitmask
    uint32_t mfc_list_stall;   // XX-L: list DMA stall status
    uint32_t mfc_atomic_stat;
    uint32_t ch_event_mask;
    uint32_t ch_event_stat;
    uint32_t decrementer;
    int      stop_status;
    uint64_t inst_count;       // XX-K: instructions executed
    uint32_t last_stop_code;
    // XX-M SPURS-ish
    int      spurs_group;      // -1 none
    uint32_t spurs_job_prio;
    uint32_t spurs_workload_id;
    bool     spurs_idle;
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
// B+J: accuracy helpers for generated / recovered code
static inline uint64_t ps3_mulhdu(uint64_t a, uint64_t b) {
#if defined(_MSC_VER)
    return __umulh(a, b);
#else
    return (uint64_t)(((unsigned __int128)a * (unsigned __int128)b) >> 64);
#endif
}
static inline int64_t ps3_mulhd(int64_t a, int64_t b) {
#if defined(_MSC_VER)
    return (int64_t)__mulh(a, b);
#else
    return (int64_t)(((__int128)a * (__int128)b) >> 64);
#endif
}
static inline uint64_t ps3_divdu(uint64_t a, uint64_t b) { return b ? a / b : ~0ull; }
static inline int64_t ps3_divd(int64_t a, int64_t b) {
    if (!b) return a >= 0 ? INT64_MAX : INT64_MIN;
    if (a == INT64_MIN && b == -1) return INT64_MIN;
    return a / b;
}
static inline uint32_t ps3_rlwinm(uint32_t rs, unsigned sh, unsigned mb, unsigned me) {
    uint32_t r = sh ? ((rs << sh) | (rs >> (32u - sh))) : rs;
    uint32_t mask = (mb <= me) ? ((0xFFFFFFFFu >> mb) & (0xFFFFFFFFu << (31u - me)))
                               : ((0xFFFFFFFFu >> mb) | (0xFFFFFFFFu << (31u - me)));
    return r & mask;
}


PS3RT_API void ps3rt_touch(uint64_t addr, size_t len);
// Tee log to stderr + timestamped file next to game EXE
PS3RT_API void ps3rt_log(const char* fmt, ...);

static constexpr uint64_t kGuestMemSize = 512ull * 1024 * 1024;
static inline bool guest_ok(const PPUContext& c, uint64_t a, size_t n) {
    return c.mem && n > 0 && a < kGuestMemSize && (a + n) <= kGuestMemSize && (a + n) >= a;
}
static inline void touch_rw(PPUContext& c, uint64_t a, size_t n) {
    (void)c; ps3rt_touch(a, n);
}

static inline uint8_t  rd8 (PPUContext& c, uint64_t a){
    if (!guest_ok(c, a, 1)) return 0;
    touch_rw(c, a, 1); return c.mem[a];
}
static inline uint16_t rd16(PPUContext& c, uint64_t a){
    if (!guest_ok(c, a, 2)) return 0;
    touch_rw(c, a, 2); uint16_t v; std::memcpy(&v, c.mem + a, 2); return bs16(v);
}
static inline uint32_t rd32(PPUContext& c, uint64_t a){
    if (!guest_ok(c, a, 4)) return 0;
    touch_rw(c, a, 4); uint32_t v; std::memcpy(&v, c.mem + a, 4); return bs32(v);
}
static inline uint64_t rd64(PPUContext& c, uint64_t a){
    if (!guest_ok(c, a, 8)) return 0;
    touch_rw(c, a, 8); uint64_t v; std::memcpy(&v, c.mem + a, 8); return bs64(v);
}
static inline void wr8 (PPUContext& c, uint64_t a, uint8_t  v){
    if (!guest_ok(c, a, 1)) return;
    touch_rw(c, a, 1); c.mem[a] = v;
}
static inline void wr16(PPUContext& c, uint64_t a, uint16_t v){
    if (!guest_ok(c, a, 2)) return;
    touch_rw(c, a, 2); v = bs16(v); std::memcpy(c.mem + a, &v, 2);
}
static inline void wr32(PPUContext& c, uint64_t a, uint32_t v){
    if (!guest_ok(c, a, 4)) return;
    touch_rw(c, a, 4); v = bs32(v); std::memcpy(c.mem + a, &v, 4);
}
static inline void wr64(PPUContext& c, uint64_t a, uint64_t v){
    if (!guest_ok(c, a, 8)) return;
    touch_rw(c, a, 8); v = bs64(v); std::memcpy(c.mem + a, &v, 8);
}

static inline void vpr_load(PPUContext& c, unsigned v, uint64_t addr) {
    addr &= ~0xFull;
    if (!guest_ok(c, addr, 16)) { std::memset(c.vpr[v], 0, 16); return; }
    touch_rw(c, addr, 16);
    std::memcpy(c.vpr[v], c.mem + addr, 16);
}
static inline void vpr_store(PPUContext& c, unsigned v, uint64_t addr) {
    addr &= ~0xFull;
    if (!guest_ok(c, addr, 16)) return;
    touch_rw(c, addr, 16);
    std::memcpy(c.mem + addr, c.vpr[v], 16);
}
static inline void vpr_and(PPUContext& c, unsigned vd, unsigned va, unsigned vb) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = (uint8_t)(c.vpr[va][i] & c.vpr[vb][i]);
}
static inline void vpr_or(PPUContext& c, unsigned vd, unsigned va, unsigned vb) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = (uint8_t)(c.vpr[va][i] | c.vpr[vb][i]);
}
static inline void vpr_xor(PPUContext& c, unsigned vd, unsigned va, unsigned vb) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = (uint8_t)(c.vpr[va][i] ^ c.vpr[vb][i]);
}
static inline void vpr_nor(PPUContext& c, unsigned vd, unsigned va, unsigned vb) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = (uint8_t)~(c.vpr[va][i] | c.vpr[vb][i]);
}
static inline void vpr_splat_u8(PPUContext& c, unsigned vd, uint8_t val) {
    for (int i = 0; i < 16; ++i) c.vpr[vd][i] = val;
}
static inline void vpr_splat_u32(PPUContext& c, unsigned vd, uint32_t val) {
    uint8_t b[4] = { (uint8_t)((val >> 24) & 0xFF), (uint8_t)((val >> 16) & 0xFF),
                     (uint8_t)((val >> 8) & 0xFF), (uint8_t)(val & 0xFF) };
    for (int w = 0; w < 4; ++w) for (int i = 0; i < 4; ++i) c.vpr[vd][w * 4 + i] = b[i];
}
static inline void vpr_splat_u16(PPUContext& c, unsigned vd, uint16_t val) {
    uint8_t b0 = (uint8_t)((val >> 8) & 0xFF), b1 = (uint8_t)(val & 0xFF);
    for (int i = 0; i < 16; i += 2) { c.vpr[vd][i] = b0; c.vpr[vd][i + 1] = b1; }
}
