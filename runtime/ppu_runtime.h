// ppu_runtime.h - header used by GENERATED code (Phase 3). Header-only on purpose.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>

#if defined(_WIN32)
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
    uint64_t lr, ctr, xer, pc;
    uint8_t  cr[8];        // each CR field: bit3=LT bit2=GT bit1=EQ bit0=SO
    uint32_t fpscr;        // floating point status and control (stub)
    uint8_t* mem;          // host pointer to guest address 0
    bool     halted;
    int      thread_id;
};

// Guest memory is big-endian. Plain helpers; fast paths can be added later.
static inline uint16_t bs16(uint16_t v){ return (uint16_t)((v>>8)|(v<<8)); }
static inline uint32_t bs32(uint32_t v){ return __builtin_bswap32(v); }
static inline uint64_t bs64(uint64_t v){ return __builtin_bswap64(v); }

static inline uint8_t  rd8 (PPUContext& c, uint64_t a){ return c.mem[a]; }
static inline uint16_t rd16(PPUContext& c, uint64_t a){ uint16_t v; __builtin_memcpy(&v,c.mem+a,2); return bs16(v); }
static inline uint32_t rd32(PPUContext& c, uint64_t a){ uint32_t v; __builtin_memcpy(&v,c.mem+a,4); return bs32(v); }
static inline uint64_t rd64(PPUContext& c, uint64_t a){ uint64_t v; __builtin_memcpy(&v,c.mem+a,8); return bs64(v); }
static inline void wr8 (PPUContext& c, uint64_t a, uint8_t v ){ c.mem[a]=v; }
static inline void wr16(PPUContext& c, uint64_t a, uint16_t v){ v=bs16(v); __builtin_memcpy(c.mem+a,&v,2); }
static inline void wr32(PPUContext& c, uint64_t a, uint32_t v){ v=bs32(v); __builtin_memcpy(c.mem+a,&v,4); }
static inline void wr64(PPUContext& c, uint64_t a, uint64_t v){ v=bs64(v); __builtin_memcpy(c.mem+a,&v,8); }

// Floating point memory helpers (big-endian bit patterns).
static inline float rd_f32(PPUContext& c, uint64_t a) {
    uint32_t bits = rd32(c, a);
    float f; std::memcpy(&f, &bits, 4);
    return f;
}
static inline double rd_f64(PPUContext& c, uint64_t a) {
    uint64_t bits = rd64(c, a);
    double d; std::memcpy(&d, &bits, 8);
    return d;
}
static inline void wr_f32(PPUContext& c, uint64_t a, float f) {
    uint32_t bits; std::memcpy(&bits, &f, 4);
    wr32(c, a, bits);
}
static inline void wr_f64(PPUContext& c, uint64_t a, double d) {
    uint64_t bits; std::memcpy(&bits, &d, 8);
    wr64(c, a, bits);
}

static inline void set_cr_signed(PPUContext& c, int f, int64_t a, int64_t b){
    c.cr[f] = (uint8_t)((a<b?8:0) | (a>b?4:0) | (a==b?2:0));
}
static inline void set_cr_unsigned(PPUContext& c, int f, uint64_t a, uint64_t b){
    c.cr[f] = (uint8_t)((a<b?8:0) | (a>b?4:0) | (a==b?2:0));
}
static inline void set_cr_fp(PPUContext& c, int f, double a, double b){
    // FL FG FE FU  (we ignore unordered for now and treat NaN as equal path)
    if (std::isnan(a) || std::isnan(b))
        c.cr[f] = 1; // FU
    else
        c.cr[f] = (uint8_t)((a<b?8:0) | (a>b?4:0) | (a==b?2:0));
}

// Decide whether a conditional branch (bc) is taken; may decrement CTR.
static inline bool bc_taken(PPUContext& c, unsigned bo, unsigned bi){
    bool ctr_ok = true, cond_ok = true;
    if (!(bo & 0x04)) { c.ctr -= 1; ctr_ok = ((c.ctr != 0) != ((bo & 0x02) != 0)); }
    if (!(bo & 0x10)) { bool bit = (c.cr[bi>>2] >> (3-(bi&3))) & 1; cond_ok = (bit == ((bo & 0x08) != 0)); }
    return ctr_ok && cond_ok;
}

// Syscall (sc) and unimplemented-instruction hooks live in ps3rt.dll.
PS3RT_API void ps3rt_syscall(PPUContext* c);
PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc);

PS3RT_API int      ps3rt_init(const char* image_path);   // 0 = ok
PS3RT_API uint8_t* ps3rt_memory(void);
PS3RT_API uint64_t ps3rt_stack_top(void);
PS3RT_API void     ps3rt_shutdown(void);

// A generated chunk covers [start,end). Returns true to keep running.
struct PPUChunk { uint64_t start, end; bool (*fn)(PPUContext&); };

static inline void ppu_run(PPUContext& c, const PPUChunk* chunks, size_t n){
    while (!c.halted) {
        const PPUChunk* hit = nullptr;
        for (size_t i = 0; i < n; ++i)
            if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { hit = &chunks[i]; break; }
        if (!hit) { c.halted = true; break; }
        hit->fn(c);
    }
}
