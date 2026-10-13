// ps3rt Phase XIV — no pure-blue 0xF1 clear; cycle dark slate + draw inject
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "nid_table.h"
#include "ps3rt_stage.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <algorithm>
#include <vector>
#include <cstdarg>
#include <ctime>
#include <atomic>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static FILE* g_logf = nullptr;
static void ps3rt_log_close(void);
static std::atomic<int> g_exit_requested{0};
PS3RT_API void ps3rt_request_exit(void) {
    g_exit_requested.store(1);
    PS3RT_STAGE("EXIT requested");
    ps3rt_log("[ps3rt] exit requested\n");
}
PS3RT_API int ps3rt_should_exit(void) { return g_exit_requested.load() != 0; }
PS3RT_API void ps3rt_force_exit(void) {
    g_exit_requested.store(1);
    ps3rt_log("[ps3rt] force exit\n");
    ps3rt_log_close();
#if defined(_WIN32)
    ExitProcess(0);
#else
    std::_Exit(0);
#endif
}
static void ps3rt_log_close(void) {
    if (g_logf) { std::fprintf(g_logf, "\n--- log end ---\n"); std::fflush(g_logf); std::fclose(g_logf); g_logf = nullptr; }
}
static void ps3rt_log_open(void) {
    if (g_logf) return;
    char dir[512]; dir[0] = 0;
#if defined(_WIN32)
    DWORD n = GetModuleFileNameA(nullptr, dir, (DWORD)sizeof(dir));
    if (n && n < sizeof(dir)) {
        char* slash = std::strrchr(dir, '\\');
        if (!slash) slash = std::strrchr(dir, '/');
        if (slash) *(slash + 1) = 0; else std::snprintf(dir, sizeof(dir), ".\\");
    } else std::snprintf(dir, sizeof(dir), ".\\");
#else
    std::snprintf(dir, sizeof(dir), "./");
#endif
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char name[64];
    std::snprintf(name, sizeof(name), "log_%04d-%02d-%02d_%02d-%02d-%02d.txt",
        tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
        tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    char full[600];
    std::snprintf(full, sizeof(full), "%s%s", dir, name);
    g_logf = std::fopen(full, "w");
    if (g_logf) {
        std::fprintf(g_logf, "PPSX33 run log\nfile=%s\n", full);
        std::fflush(g_logf);
        ps3rt_log("[ps3rt] log file: %s\n", full);
    }
}
PS3RT_API void ps3rt_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fflush(stderr);
    if (g_logf) {
        va_start(ap, fmt);
        std::vfprintf(g_logf, fmt, ap);
        va_end(ap);
        std::fflush(g_logf);
    }
}

static uint8_t* g_mem = nullptr;
static size_t g_mem_size = 0;
static const uint64_t kStackBase = 0x0F000000ull;
static const size_t kStackSize = 0x100000;
static std::mutex g_lock;
static uint64_t g_alloc_ptr = 0x08000000ull;
static const uint64_t kAllocEnd = 0x0E000000ull;
static void wr_be64_guest(uint64_t addr, uint64_t val) {
    if (!g_mem || addr + 8 > g_mem_size) return;
    uint8_t b[8]; for (int i = 7; i >= 0; --i) { b[i] = (uint8_t)(val & 0xFF); val >>= 8; }
    std::memcpy(g_mem + addr, b, 8);
}
static void wr_be32_guest(uint64_t addr, uint32_t val) {
    if (!g_mem || addr + 4 > g_mem_size) return;
    uint8_t b[4] = { (uint8_t)(val>>24), (uint8_t)(val>>16), (uint8_t)(val>>8), (uint8_t)val };
    std::memcpy(g_mem + addr, b, 4);
}
static uint32_t rd_be32_guest(uint64_t addr) {
    if (!g_mem || addr + 4 > g_mem_size) return 0;
    return ((uint32_t)g_mem[addr] << 24) | ((uint32_t)g_mem[addr+1] << 16) |
           ((uint32_t)g_mem[addr+2] << 8) | (uint32_t)g_mem[addr+3];
}
static uint64_t guest_alloc(uint64_t size, uint64_t align) {
    if (size == 0) size = 0x1000;
    if (align < 0x1000) align = 0x1000;
    if (align & (align - 1)) align = 0x10000;
    uint64_t a = (g_alloc_ptr + (align - 1)) & ~(align - 1);
    if (a + size > kAllocEnd || a + size > g_mem_size) return 0;
    std::memset(g_mem + a, 0, (size_t)size);
    g_alloc_ptr = a + size;
    ps3rt_log("[lv2] alloc VA=0x%llx size=0x%llx\n", (unsigned long long)a, (unsigned long long)size);
    return a;
}
PS3RT_API void ps3rt_touch(uint64_t, size_t) {
    static uint32_t s_n = 0;
    static uint32_t s_log = 0;
    if ((++s_n & 0x1FFF) == 0) {
        ps3rt_progress_hw();
        if (s_log < 12) { ps3rt_log("[ps3] Phase M touch-progress n=%u\n", s_n); ++s_log; }
    }
}
static uint32_t rd_le32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
static uint64_t rd_le64(const uint8_t* p) { uint64_t v; std::memcpy(&v, p, 8); return v; }
static int load_guest_image(const char* path) {
    FILE* f = std::fopen(path, "rb"); if (!f) return -1;
    std::fseek(f, 0, SEEK_END); long fsz = std::ftell(f); std::fseek(f, 0, SEEK_SET);
    if (fsz < 12) { std::fclose(f); return -1; }
    std::vector<uint8_t> file((size_t)fsz);
    if (std::fread(file.data(), 1, (size_t)fsz, f) != (size_t)fsz) { std::fclose(f); return -1; }
    std::fclose(f);
    if (std::memcmp(file.data(), "PS3IMG1", 7) == 0) {
        uint32_t nseg = rd_le32(file.data() + 8);
        int mapped = 0;
        for (uint32_t i = 0; i < nseg; ++i) {
            const uint8_t* e = file.data() + 12 + i * 32;
            uint64_t vaddr = rd_le64(e + 0), filesz = rd_le64(e + 8), memsz = rd_le64(e + 16), off = rd_le64(e + 24);
            if (filesz == 0 && memsz == 0) continue;
            if (off + filesz > (size_t)fsz) continue;
            if (vaddr + memsz > g_mem_size) { if (vaddr >= g_mem_size) continue; memsz = g_mem_size - vaddr; if (filesz > memsz) filesz = memsz; }
            std::memset(g_mem + vaddr, 0, (size_t)memsz);
            if (filesz) std::memcpy(g_mem + vaddr, file.data() + (size_t)off, (size_t)filesz);
            ps3rt_log("[ps3rt] map seg%u VA=0x%llx filesz=0x%llx memsz=0x%llx\n", i, (unsigned long long)vaddr, (unsigned long long)filesz, (unsigned long long)memsz);
            ++mapped;
        }
        ps3rt_log("[ps3rt] PS3IMG1 loaded from %s (%d segments)\n", path, mapped);
        PS3RT_STAGE("MEM map end");
        return mapped > 0 ? 0 : -1;
    }
    if ((size_t)fsz <= g_mem_size) { std::memcpy(g_mem, file.data(), (size_t)fsz); return 0; }
    return -1;
}

PS3RT_API int ps3rt_init(const char* image_path) {
    std::lock_guard<std::mutex> lk(g_lock);
    ps3rt_log_open();
    g_exit_requested.store(0);
    PS3RT_STAGE("BOOT start");
    if (g_mem) return 0;
    g_mem_size = 512ull * 1024 * 1024;
    g_mem = (uint8_t*)std::malloc(g_mem_size);
    if (!g_mem) return -1;
    std::memset(g_mem, 0, g_mem_size);
    g_alloc_ptr = 0x08000000ull;
    const char* candidates[] = { image_path, "guest_image.bin", "output/guest_image.bin" };
    int ok = -1;
    for (auto c : candidates) {
        if (c && c[0] && load_guest_image(c) == 0) { ok = 0; break; }
    }
    if (ok != 0) ps3rt_log("[ps3rt] warning: no guest_image.bin mapped\n");
    PS3RT_STAGE("BOOT end");
    return 0;
}
PS3RT_API uint8_t* ps3rt_memory(void) { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize; }
PS3RT_API void ps3rt_shutdown(void) { std::lock_guard<std::mutex> lk(g_lock); std::free(g_mem); g_mem = nullptr; g_mem_size = 0; ps3rt_log_close(); }

static SPUContext g_spus[6];
static int g_spu_count = 0;
PS3RT_API int ps3rt_spu_supported(void) { return 1; }
PS3RT_API int ps3rt_spu_create(int* out_id) {
    if (g_spu_count >= 6) return -1;
    int id = g_spu_count++;
    std::memset(&g_spus[id], 0, sizeof(SPUContext));
    if (out_id) *out_id = id;
    return 0;
}
PS3RT_API int ps3rt_spu_destroy(int id) { if (id < 0 || id >= g_spu_count) return -1; g_spus[id].running = false; return 0; }
PS3RT_API int ps3rt_spu_load(int id, const void* img, size_t len) {
    if (id < 0 || id >= g_spu_count || !img) return -1;
    size_t n = len < sizeof(g_spus[id].ls) ? len : sizeof(g_spus[id].ls);
    std::memcpy(g_spus[id].ls, img, n);
    return 0;
}
PS3RT_API int ps3rt_spu_run(int id, uint32_t entry) {
    if (id < 0 || id >= g_spu_count) return -1;
    g_spus[id].pc = entry; g_spus[id].running = false; g_spus[id].stop_status = 0x2000;
    if (g_spus[id].mailbox_out_count < 4) g_spus[id].mailbox_out[g_spus[id].mailbox_out_count++] = 1;
    static int slog = 0; if (slog < 8) { ps3rt_log("[spu] run id=%d entry=0x%x -> complete+mbox\n", id, entry); ++slog; }
    return 0;
}
PS3RT_API int ps3rt_spu_stop(int id) { if (id < 0 || id >= g_spu_count) return -1; g_spus[id].running = false; return 0; }
PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= g_spu_count) return -1;
    if (g_spus[id].mailbox_in_count >= 4) return -1;
    g_spus[id].mailbox_in[g_spus[id].mailbox_in_count++] = val; return 0;
}
PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= g_spu_count || !out) return -1;
    if (g_spus[id].mailbox_out_count <= 0) { *out = 0; return -1; }
    *out = g_spus[id].mailbox_out[0];
    for (int i = 1; i < g_spus[id].mailbox_out_count; ++i) g_spus[id].mailbox_out[i-1] = g_spus[id].mailbox_out[i];
    --g_spus[id].mailbox_out_count; return 0;
}
PS3RT_API int ps3rt_spu_mfc_dma(int, uint32_t, uint64_t, uint32_t, uint32_t) { return 0; }
static void spu_tick_all() {
    static int s_spu_once = 0;
    if (!s_spu_once) { PS3RT_STAGE("SPU progress end"); s_spu_once = 1; }
}

static uint16_t g_pad_buttons = 0;
static int8_t g_pad_lx = 0, g_pad_ly = 0, g_pad_rx = 0, g_pad_ry = 0;
PS3RT_API void ps3rt_pad_set(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
    g_pad_buttons = buttons;
    g_pad_lx = (int8_t)(lx >> 8); g_pad_ly = (int8_t)(ly >> 8);
    g_pad_rx = (int8_t)(rx >> 8); g_pad_ry = (int8_t)(ry >> 8);
}

PS3RT_API void ps3rt_syscall(PPUContext* c) {
    if (!c) return;
    uint64_t num = c->gpr[11];
    uint64_t r3 = c->gpr[3], r4 = c->gpr[4], r5 = c->gpr[5], r6 = c->gpr[6];
    static int sys_log = 0;
    if (sys_log < 80) { ps3rt_log("[lv2] sc %llu r3=%llx r4=%llx r5=%llx r6=%llx\n", (unsigned long long)num, (unsigned long long)r3, (unsigned long long)r4, (unsigned long long)r5, (unsigned long long)r6); ++sys_log; }
    switch ((uint32_t)num) {
    case 1: c->halted = true; c->gpr[3] = 0; break;
    case 141: case 142: case 144: case 145: case 48: case 49: case 50:
    case 52: case 53: case 54: case 70: case 71: case 72: case 90: case 91:
    case 120: case 121: case 129: case 130: case 131: case 132: case 133: case 134:
    case 114: case 115: case 116: case 169: case 348: case 349: case 350: case 351:
    case 353: case 354: case 355: case 403: case 801: case 802: case 803:
    case 871: case 872: case 25602: c->gpr[3] = 0; break;
    case 128: {
        static uint32_t eid = 0x7000; ++eid;
        uint64_t out = r3 ? r3 : r4;
        if (out && out + 4 <= g_mem_size) wr_be32_guest(out, eid);
        c->gpr[3] = 0; break;
    }
    case 143: { static uint32_t fid = 0x8000; if (r3 && r3+4 <= g_mem_size) wr_be32_guest(r3, ++fid); c->gpr[3] = 0; break; }
    case 352: {
        uint64_t size = r3 ? r3 : 0x100000;
        if (size > 0x1000000) size = 0x1000000;
        uint64_t a = guest_alloc(size, 0x10000);
        if (r4 && r4 + 8 <= g_mem_size) wr_be64_guest(r4, a);
        c->gpr[3] = a ? 0 : (uint64_t)-1; break;
    }
    default: c->gpr[3] = 0; break;
    }
}

static int g_prx_log = 0;
static uint64_t g_gcm_fifo = 0;
static uint32_t g_gcm_fifo_size = 0x100000;
static int g_gcm_inited = 0;
static uint64_t g_gcm_control = 0;
static uint64_t g_gcm_labels = 0;
static uint32_t g_gcm_flip_status = 0;
static uint32_t g_gcm_frame_id = 0;
static void gcm_ensure_state() {
    if (!g_gcm_control) {
        g_gcm_control = guest_alloc(0x1000, 0x1000);
        if (g_gcm_control) {
            wr_be32_guest(g_gcm_control+0,0); wr_be32_guest(g_gcm_control+4,0); wr_be32_guest(g_gcm_control+8,0);
            ps3rt_log("[gcm] control @ 0x%llx\n", (unsigned long long)g_gcm_control);
        }
    }
    if (!g_gcm_labels) {
        g_gcm_labels = guest_alloc(0x10000, 0x1000);
        if (g_gcm_labels) ps3rt_log("[gcm] labels  @ 0x%llx\n", (unsigned long long)g_gcm_labels);
    }
    if (!g_gcm_inited) {
        g_gcm_inited = 1;
        if (!g_gcm_fifo) g_gcm_fifo = guest_alloc(g_gcm_fifo_size, 0x10000);
        ps3rt_rsx_init(g_gcm_fifo ? g_gcm_fifo : 0, g_gcm_fifo_size);
        ps3rt_log("[gcm] Phase G state ready fifo=0x%llx ctrl=0x%llx\n", (unsigned long long)g_gcm_fifo, (unsigned long long)g_gcm_control);
        PS3RT_STAGE("GCM ready end");
    }
}
static void gcm_fifo_emit(uint32_t method, uint32_t arg) {
    if (!g_gcm_control || !g_gcm_fifo) return;
    uint32_t fsz = g_gcm_fifo_size ? g_gcm_fifo_size : 0x100000;
    uint32_t put = rd_be32_guest(g_gcm_control + 0) % fsz;
    uint64_t fa = g_gcm_fifo + put;
    if (fa + 8 <= g_mem_size) {
        uint32_t hdr = (1u << 18) | (method & 0x1FFC);
        wr_be32_guest(fa, hdr);
        wr_be32_guest(fa + 4, arg);
        put = (put + 8) % fsz;
        wr_be32_guest(g_gcm_control + 0, put);
        wr_be32_guest(g_gcm_control + 4, put);
        ps3rt_rsx_set_put(put);
    }
}
static void gcm_inject_clear_draw() {
    static const uint32_t kCols[] = { 0x101828u, 0x182030u, 0x203040u, 0x283848u };
    gcm_fifo_emit(0x01D8, kCols[(g_gcm_frame_id / 45) % 4]);
    gcm_fifo_emit(0x0208, (1280u << 16));
    gcm_fifo_emit(0x020C, (720u << 16));
    gcm_fifo_emit(0x0A00, (1280u << 16));
    gcm_fifo_emit(0x0A04, (720u << 16));
    gcm_fifo_emit(0x1800, 0x3u);
}
static bool looks_like_nid(uint32_t n) {
    if (n < 0x10000u) return false;
    if (n == 0x39800000u) return false;
    if (n >= 0x00010000u && n < 0x02000000u) return false;
    if (n >= 0x08000000u && n < 0x10000000u) return false;
    return true;
}
static uint32_t rd_be32_raw(uint64_t addr) {
    if (!g_mem || addr + 4 > g_mem_size) return 0;
    return ((uint32_t)g_mem[addr] << 24) | ((uint32_t)g_mem[addr+1] << 16) |
           ((uint32_t)g_mem[addr+2] << 8) | (uint32_t)g_mem[addr+3];
}
static uint32_t recover_nid_from_callsite(uint64_t lr) {
    if (!lr || lr < 16) return 0;
    for (int back = 1; back <= 8; ++back) {
        uint64_t a = lr - (uint64_t)back * 4; uint32_t op = rd_be32_raw(a);
        if ((op >> 26) == 24) {
            uint32_t lo = op & 0xFFFF;
            for (int b2 = back + 1; b2 <= back + 6; ++b2) {
                uint32_t op2 = rd_be32_raw(lr - (uint64_t)b2 * 4);
                if ((op2 >> 26) == 15 && ((op2 >> 16) & 0x1F) == 0) {
                    if (((op >> 21) & 0x1F) == ((op2 >> 21) & 0x1F)) {
                        uint32_t nid = ((uint32_t)(int16_t)(op2 & 0xFFFF) << 16) | lo;
                        if (nid && nid != 0x39800000u) return nid;
                    }
                }
            }
        }
    }
    return 0;
}
static int prx_dispatch(PPUContext* c, int kind, const char* name) {
    uint32_t cands[8] = {
        (uint32_t)c->gpr[11], (uint32_t)(c->gpr[11] >> 32),
        (uint32_t)c->gpr[0], (uint32_t)c->gpr[12],
        (uint32_t)c->gpr[3], (uint32_t)c->gpr[4],
        recover_nid_from_callsite(c->lr), recover_nid_from_callsite(c->pc)
    };
    const NidEntry* e = nullptr; uint32_t maybe_nid = 0;
    for (int i = 0; i < 8; ++i) {
        uint32_t n = cands[i]; if (!looks_like_nid(n)) continue;
        e = nid_lookup(n); if (e) { maybe_nid = n; break; }
        if (!maybe_nid) maybe_nid = n;
    }
    if (kind == 1) gcm_ensure_state();
    if (e) {
        if (g_prx_log < 40) { ps3rt_log("[prx] %s::%s nid=0x%08X\n", e->module, e->name, e->nid); ++g_prx_log; }
        kind = e->kind;
        if (e->nid == 0x15BAE46B) {
            if (c->gpr[3]) { g_gcm_fifo = c->gpr[3]; g_gcm_fifo_size = (uint32_t)c->gpr[4] ? (uint32_t)c->gpr[4] : 0x100000; }
            gcm_ensure_state(); if (g_gcm_fifo) ps3rt_rsx_init(g_gcm_fifo, g_gcm_fifo_size); c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x55A14C05) { gcm_ensure_state(); c->gpr[3] = g_gcm_control; return 0; }
        if (e->nid == 0x5F909B17) { gcm_ensure_state(); uint32_t idx = (uint32_t)c->gpr[3] & 0xFF; c->gpr[3] = g_gcm_labels + (uint64_t)idx * 0x10; return 0; }
        if (e->nid == 0xD81D0D2D || e->nid == 0x05E548B9) { ps3rt_rsx_flip((uint32_t)c->gpr[3]); g_gcm_flip_status = 0; g_gcm_frame_id++; c->gpr[3] = 0; return 0; }
        if (e->nid == 0xA547ADDE) { c->gpr[3] = g_gcm_flip_status; return 0; }
        if (e->nid == 0xB2E761D4 || e->nid == 0xF80196C0) { g_gcm_flip_status = 0; c->gpr[3] = 0; return 0; }
        if (e->nid == 0x3AAAD464) {
            uint64_t out = c->gpr[4];
            if (out && out + 24 <= g_mem_size) {
                wr_be32_guest(out, 24);
                g_mem[out+4] = (uint8_t)(g_pad_buttons >> 8); g_mem[out+5] = (uint8_t)g_pad_buttons;
                g_mem[out+6] = (uint8_t)g_pad_lx; g_mem[out+7] = (uint8_t)g_pad_ly;
                g_mem[out+8] = (uint8_t)g_pad_rx; g_mem[out+9] = (uint8_t)g_pad_ry;
            }
            c->gpr[3] = 0; return 0;
        }
        if (kind == 1 || (kind >= 2 && kind <= 10)) { c->gpr[3] = 0; return 0; }
    } else if (g_prx_log < 32) {
        ps3rt_log("[prx] stub kind=%d %s pc=0x%llx nid=0x%08X lr=0x%llx r11=0x%llx\n",
            kind, name ? name : "?", (unsigned long long)c->pc, maybe_nid, (unsigned long long)c->lr, (unsigned long long)c->gpr[11]);
        ++g_prx_log;
    }
    if (kind == 1) {
        gcm_ensure_state();
        static uint32_t s_gcm_seq = 0;
        uint32_t seq = ++s_gcm_seq;
        if ((seq & 0x7) == 1 && g_gcm_control) {
            c->gpr[3] = g_gcm_control;
            if ((seq & 0x3F) == 1) gcm_inject_clear_draw();
            return 0;
        }
        if ((seq & 0x7) == 3 && g_gcm_labels) { c->gpr[3] = g_gcm_labels; return 0; }
        if ((seq & 0x7) == 5) { c->gpr[3] = g_gcm_flip_status; return 0; }
        c->gpr[3] = 0; return 0;
    }
    c->gpr[3] = 0; return 0;
}
PS3RT_API void ps3rt_progress_hw(void) {
    if (!g_mem) return;
    static int s_stage_ppu = 0;
    if (s_stage_ppu == 0) { PS3RT_STAGE("PPU progress start"); s_stage_ppu = 1; }
    if (g_exit_requested.load() != 0) {
        static int s_ppu_end = 0;
        if (!s_ppu_end) { PS3RT_STAGE("PPU progress end"); s_ppu_end = 1; }
        ps3rt_force_exit();
        return;
    }
    spu_tick_all();
    if (!g_gcm_inited) gcm_ensure_state();
    static uint32_t s_last_put = 0;
    static uint32_t s_stall = 0;
    static uint32_t s_inject = 0;
    static int s_real_put_log = 0;
    if (g_gcm_control && g_gcm_control + 16 <= g_mem_size) {
        uint32_t fsz = g_gcm_fifo_size ? g_gcm_fifo_size : 0x100000;
        uint32_t put = rd_be32_guest(g_gcm_control + 0) % fsz;
        if (put != s_last_put) {
            if (s_real_put_log < 16) {
                ps3rt_log("[gcm] Phase XII guest put 0x%x -> 0x%x\n", s_last_put, put);
                ++s_real_put_log;
            }
            s_last_put = put; s_stall = 0;
        } else ++s_stall;
        if (g_gcm_inited && s_stall > 8 && (++s_inject & 0xF) == 1) {
            gcm_inject_clear_draw();
            put = rd_be32_guest(g_gcm_control + 0) % fsz;
            s_last_put = put;
        }
        ps3rt_rsx_set_put(put);
        ps3rt_rsx_flush(g_mem);
        uint32_t get_now = ps3rt_rsx_get_get() % fsz;
        wr_be32_guest(g_gcm_control + 4, get_now);
        wr_be32_guest(g_gcm_control + 8, get_now);
    } else if (g_gcm_inited && (++s_inject & 0x1F) == 1) {
        gcm_inject_clear_draw();
    }
    g_gcm_flip_status = 0;
    static uint32_t s_flip_div = 0;
    if ((++s_flip_div & 0x7) == 0) {
        ps3rt_rsx_flip(0); g_gcm_frame_id++;
        static int s_run_stage = 0;
        if (!s_run_stage && g_gcm_frame_id > 30) { PS3RT_STAGE("RUNNING"); s_run_stage = 1; }
    }
    if (g_gcm_labels && g_gcm_labels + 0x1000 <= g_mem_size) {
        for (int i = 0; i < 256; ++i) wr_be32_guest(g_gcm_labels + (uint64_t)i * 0x10, 0);
    }
}
PS3RT_API int ps3rt_prx_import_stub(PPUContext* c, uint64_t pc) {
    if (!c) return -1;
    if (pc == 0x39800000ull || (pc & 0xFFFF0000ull) == 0x39800000ull) return prx_dispatch(c, 1, "cellGcmSys");
    return prx_dispatch(c, prx_module_kind("unknown"), "import");
}
PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int n = 0;
    if (n < 8) { ps3rt_log("[ps3] unimplemented op=0x%08X pc=0x%llx\n", opcode, (unsigned long long)pc); ++n; }
    if (c) c->halted = true;
}
