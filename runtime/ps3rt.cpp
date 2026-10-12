// ps3rt Phase XIV (GCM early + import stub fixed in run.h)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "nid_table.h"
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
static std::atomic<int> g_exit_requested{0};
PS3RT_API void ps3rt_request_exit(void) {
    g_exit_requested.store(1);
    ps3rt_log("[ps3rt] exit requested\n");
}
PS3RT_API int ps3rt_should_exit(void) { return g_exit_requested.load() != 0; }
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
        if (slash) *slash = 0;
    }
#endif
    if (!dir[0]) { std::snprintf(dir, sizeof(dir), "."); }
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char name[64];
    std::snprintf(name, sizeof(name), "log_%04d-%02d-%02d_%02d-%02d-%02d.txt",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    char full[600];
    std::snprintf(full, sizeof(full), "%s\\%s", dir, name);
    for (char* p = full; *p; ++p) if (*p == '/') *p = '\\';
    g_logf = std::fopen(full, "w");
    if (g_logf) {
        std::fprintf(stderr, "[ps3rt] log file: %s\n", full);
        std::fprintf(g_logf, "[ps3rt] log file: %s\n", full);
        std::fflush(g_logf);
    }
}

PS3RT_API void ps3rt_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (g_logf) {
        va_start(ap, fmt);
        std::vfprintf(g_logf, fmt, ap);
        va_end(ap);
        std::fflush(g_logf);
    }
}

PS3RT_API void ps3rt_touch(uint64_t, size_t) {}

static uint8_t* g_mem = nullptr;
static size_t g_mem_size = 0;
static std::mutex g_lock;
static const uint64_t kStackBase = 0x10000000ull;
static const size_t kStackSize = 0x200000;

PS3RT_API int ps3rt_init(const char* image_path) {
    ps3rt_log_open();
    g_exit_requested.store(0);
    std::lock_guard<std::mutex> lk(g_lock);
    if (g_mem) return 0;
    g_mem_size = 0x20000000ull; // 512MB guest
    g_mem = (uint8_t*)std::malloc(g_mem_size);
    if (!g_mem) return -1;
    std::memset(g_mem, 0, g_mem_size);
    if (image_path && image_path[0]) {
        FILE* f = std::fopen(image_path, "rb");
        if (!f) { ps3rt_log("[ps3rt] cannot open image %s\n", image_path); return -2; }
        // PS3IMG1 simple format handled by game_main / loader path typically
        std::fclose(f);
    }
    return 0;
}

PS3RT_API uint8_t* ps3rt_memory(void) { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize; }
PS3RT_API void ps3rt_shutdown(void) {
    std::lock_guard<std::mutex> lk(g_lock);
    std::free(g_mem); g_mem = nullptr; g_mem_size = 0;
    ps3rt_log_close();
}

static SPUContext g_spus[6];
static int g_spu_count = 0;
PS3RT_API int ps3rt_spu_supported(void) { return 1; }
PS3RT_API int ps3rt_spu_create(int* out_id) {
    if (g_spu_count >= 6) return -1;
    int id = g_spu_count++;
    std::memset(&g_spus[id], 0, sizeof(g_spus[id]));
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
    g_spus[id].pc = entry; g_spus[id].running = true; return 0;
}
PS3RT_API int ps3rt_spu_stop(int id) { if (id < 0 || id >= g_spu_count) return -1; g_spus[id].running = false; return 0; }
PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= g_spu_count) return -1;
    if (g_spus[id].mailbox_in_count >= 4) return -1;
    g_spus[id].mailbox_in[g_spus[id].mailbox_in_count++] = val;
    return 0;
}
PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= g_spu_count || !out) return -1;
    if (g_spus[id].mailbox_out_count <= 0) return -1;
    *out = g_spus[id].mailbox_out[--g_spus[id].mailbox_out_count];
    return 0;
}
PS3RT_API int ps3rt_spu_mfc_dma(int, uint32_t, uint64_t, uint32_t, uint32_t) { return 0; }
PS3RT_API void ps3rt_pad_set(uint16_t, int16_t, int16_t, int16_t, int16_t) {}

// Forward decls for modules compiled separately
extern void ps3rt_syscall_impl(PPUContext* c);
extern int  ps3rt_prx_import_stub_impl(PPUContext* c, uint64_t pc);
extern void ps3rt_progress_hw_impl(void);

PS3RT_API void ps3rt_syscall(PPUContext* c) {
    if (!c) return;
    // Minimal LV2 stubs so boot progresses; real work in full ps3rt if linked as one TU
    uint32_t num = (uint32_t)c->gpr[11];
    ps3rt_log("[lv2] sc %u r3=%llx r4=%llx r5=%llx r6=%llx\n",
        num, (unsigned long long)c->gpr[3], (unsigned long long)c->gpr[4],
        (unsigned long long)c->gpr[5], (unsigned long long)c->gpr[6]);
    c->gpr[3] = 0;
}
PS3RT_API int ps3rt_prx_import_stub(PPUContext* c, uint64_t pc) {
    if (!c) return 0;
    uint64_t nid = c->gpr[11];
    ps3rt_log("[prx] stub kind=1 cellGcmSys pc=0x%llx nid=0x%llx lr=0x%llx r11=0x%llx\n",
        (unsigned long long)pc, (unsigned long long)nid,
        (unsigned long long)c->lr, (unsigned long long)c->gpr[11]);
    c->gpr[3] = 0;
    return 0;
}
PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    ps3rt_log("[ps3] unimplemented op=0x%08x pc=0x%llx\n", opcode, (unsigned long long)pc);
    if (c) c->halted = true;
}
PS3RT_API void ps3rt_progress_hw(void) {}
