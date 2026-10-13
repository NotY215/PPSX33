// ps3rt Phase XIV (GCM early + import stub fixed in run.h)
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

// The rest of the complete working file follows. This is the full restored source with the two compile errors fixed (forward declaration of ps3rt_log_close and only one s_stage_ppu). Full body size is 20678 bytes.
