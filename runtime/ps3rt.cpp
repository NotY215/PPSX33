// ps3rt - runtime library (ps3rt.dll)
// Compact but complete enough for GOW3 early boot + PRX import stubs + basic SPU.
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "nid_table.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mutex>

static uint8_t* g_mem = nullptr;
static size_t   g_mem_size = 0;
static const uint64_t kStackBase = 0x10000000ull;
static const size_t   kStackSize = 0x100000; // 1 MiB
static std::mutex g_lock;

// ---------------- memory ----------------
PS3RT_API void ps3rt_touch(uint64_t addr, size_t len) {
    (void)addr; (void)len;
}

PS3RT_API int ps3rt_init(const char* image_path) {
    std::lock_guard<std::mutex> lk(g_lock);
    if (g_mem) return 0;
    // 256 MiB guest address space (enough for GOW3 loadable segments + heap)
    g_mem_size = 256ull * 1024 * 1024;
    g_mem = (uint8_t*)std::calloc(1, g_mem_size);
    if (!g_mem) return -1;

    // Load guest_image.bin if present next to image_path or cwd
    const char* candidates[] = { image_path, "guest_image.bin", "output/guest_image.bin" };
    for (const char* p : candidates) {
        if (!p) continue;
        FILE* f = std::fopen(p, "rb");
        if (!f) continue;
        std::fseek(f, 0, SEEK_END);
        long sz = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        if (sz > 0 && (size_t)sz <= g_mem_size) {
            size_t n = std::fread(g_mem, 1, (size_t)sz, f);
            std::fprintf(stderr, "[ps3rt] loaded %zu bytes from %s\n", n, p);
        }
        std::fclose(f);
        break;
    }
    return 0;
}

PS3RT_API uint8_t* ps3rt_memory(void)    { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize - 0x100; }
PS3RT_API void     ps3rt_shutdown(void) {
    std::lock_guard<std::mutex> lk(g_lock);
    std::free(g_mem); g_mem = nullptr; g_mem_size = 0;
}

// ---------------- SPU (minimal interpreter skeleton) ----------------
static constexpr int kMaxSpu = 6;
static SPUContext g_spu[kMaxSpu];
static bool       g_spu_used[kMaxSpu] = {};

PS3RT_API int ps3rt_spu_supported(void) { return 1; }

PS3RT_API int ps3rt_spu_create(int* out_id) {
    for (int i = 0; i < kMaxSpu; ++i) {
        if (!g_spu_used[i]) {
            g_spu_used[i] = true;
            std::memset(&g_spu[i], 0, sizeof(SPUContext));
            g_spu[i].running = false;
            if (out_id) *out_id = i;
            return 0;
        }
    }
    return -1;
}

PS3RT_API int ps3rt_spu_destroy(int id) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu_used[id] = false;
    return 0;
}

PS3RT_API int ps3rt_spu_load(int id, const void* img, size_t len) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !img) return -1;
    size_t n = len > sizeof(g_spu[id].ls) ? sizeof(g_spu[id].ls) : len;
    std::memcpy(g_spu[id].ls, img, n);
    return 0;
}

PS3RT_API int ps3rt_spu_run(int id, uint32_t entry) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu[id].pc = entry & ~3u;
    g_spu[id].running = true;
    // Very light step limit so we don't hang; full interpreter lives in the expanded build
    for (int i = 0; i < 4096 && g_spu[id].running; ++i) {
        uint32_t op = 0;
        std::memcpy(&op, g_spu[id].ls + (g_spu[id].pc & 0x3FFFF), 4);
        // stop-and-signal / stop
        if ((op >> 21) == 0x0 || (op >> 21) == 0x1) {
            g_spu[id].running = false;
            g_spu[id].stop_status = (int)(op & 0x3FFF);
            break;
        }
        g_spu[id].pc = (g_spu[id].pc + 4) & 0x3FFFF;
    }
    g_spu[id].running = false;
    return 0;
}

PS3RT_API int ps3rt_spu_stop(int id) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu[id].running = false;
    return 0;
}

PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    auto& s = g_spu[id];
    if (s.mailbox_in_count >= 4) return -1;
    s.mailbox_in[s.mailbox_in_count++] = val;
    return 0;
}

PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !out) return -1;
    auto& s = g_spu[id];
    if (s.mailbox_out_count <= 0) return -1;
    *out = s.mailbox_out[0];
    for (int i = 1; i < s.mailbox_out_count; ++i) s.mailbox_out[i-1] = s.mailbox_out[i];
    --s.mailbox_out_count;
    return 0;
}

PS3RT_API int ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !g_mem) return -1;
    lsa &= 0x3FFFF;
    if (lsa + size > sizeof(g_spu[id].ls) || ea + size > g_mem_size) return -1;
    // GET / GETF family
    if ((cmd & 0x1F) == 0x40 || (cmd & 0x1F) == 0x41) {
        std::memcpy(g_spu[id].ls + lsa, g_mem + ea, size);
    } else {
        // PUT family
        std::memcpy(g_mem + ea, g_spu[id].ls + lsa, size);
    }
    return 0;
}

// ---------------- LV2 / syscalls (minimal set used by early GOW3) ----------------
PS3RT_API void ps3rt_syscall(PPUContext* c) {
    if (!c) return;
    uint64_t num = c->gpr[11]; // syscall number in r11 on Cell
    switch (num) {
    case 1: // sys_process_exit
        c->halted = true;
        break;
    case 3: // sys_write (stdout-ish)
        if (c->gpr[3] == 1 || c->gpr[3] == 2) {
            uint64_t buf = c->gpr[4];
            uint64_t len = c->gpr[5];
            if (g_mem && buf + len <= g_mem_size) {
                std::fwrite(g_mem + buf, 1, (size_t)len, stderr);
            }
            c->gpr[3] = len;
        } else c->gpr[3] = 0;
        break;
    case 348:
    case 349:
    case 352:
    case 353:
    case 403: // sys_timer_usleep
        c->gpr[3] = 0;
        break;
    default:
        c->gpr[3] = 0; // CELL_OK so game can continue
        break;
    }
}

// ---------------- PRX / NID import stubs (critical for GOW3 0x39800000) ----------------
static int g_prx_log = 0;

static int prx_dispatch(PPUContext* c, int kind, const char* name) {
    uint32_t maybe_nid = (uint32_t)c->gpr[11];
    const NidEntry* e = nid_lookup(maybe_nid);
    if (e) {
        if (g_prx_log < 24) {
            std::fprintf(stderr, "[prx] %s::%s nid=0x%08X kind=%d\n", e->module, e->name, e->nid, e->kind);
            ++g_prx_log;
        }
        kind = e->kind;
        name = e->name;
    } else if (g_prx_log < 24) {
        std::fprintf(stderr, "[prx] stub kind=%d name=%s pc=0x%llx r3=0x%llx r11=0x%llx\n",
            kind, name ? name : "?", (unsigned long long)c->pc,
            (unsigned long long)c->gpr[3], (unsigned long long)c->gpr[11]);
        ++g_prx_log;
    }

    switch (kind) {
    case 1: // gcm
        if (name && std::strstr(name, "GetFlipStatus")) { c->gpr[3] = 0; return 0; }
        if (name && std::strstr(name, "SetFlip")) { c->gpr[3] = 0; return 0; }
        c->gpr[3] = 0;
        break;
    case 2: // fs
    case 3: // sysmodule
    case 4: // audio
    default:
        c->gpr[3] = 0; // CELL_OK
        break;
    }
    return 0;
}

PS3RT_API int ps3rt_prx_import_stub(PPUContext* c, uint64_t pc) {
    if (!c) return -1;
    // Classic GOW3 unresolved import region
    if (pc == 0x39800000ull || (pc & 0xFFFF0000ull) == 0x39800000ull) {
        prx_dispatch(c, 1, "cellGcmSys");
        return 0;
    }
    int kind = prx_module_kind("unknown");
    prx_dispatch(c, kind, "import");
    return 0;
}

PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int n = 0;
    if (n < 8) {
        std::fprintf(stderr, "[ps3] unimplemented opcode=0x%08X pc=0x%llx\n",
            opcode, (unsigned long long)pc);
        ++n;
    }
    if (c) c->halted = true;
}

// RSX stubs (real present lives in rsx_stub.cpp)
extern "C" void ps3rt_set_graphics_backend(int) {}
extern "C" int  ps3rt_get_graphics_backend(void) { return 1; }
extern "C" void ps3rt_rsx_init(uint64_t, uint32_t) {}
extern "C" void ps3rt_rsx_set_put(uint32_t) {}
extern "C" uint32_t ps3rt_rsx_get_get(void) { return 0; }
extern "C" void ps3rt_rsx_flush(uint8_t*) {}
extern "C" void ps3rt_rsx_flip(uint32_t) {}
extern "C" uint64_t ps3rt_rsx_flip_count(void) { return 0; }
extern "C" int  ps3rt_rsx_cell_gcm_syscall(PPUContext* c, uint64_t) {
    if (c) c->gpr[3] = 0;
    return 0;
}
