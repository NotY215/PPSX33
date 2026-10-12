#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdarg>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static FILE* g_logf = nullptr;

extern "C" void ps3rt_log_close(void) {
    if (g_logf) {
        std::fprintf(g_logf, "\n--- log end ---\n");
        std::fflush(g_logf);
        std::fclose(g_logf);
        g_logf = nullptr;
    }
}

extern "C" void ps3rt_log_open(void) {
    if (g_logf) return;
    char dir[512];
    dir[0] = 0;
#if defined(_WIN32)
    DWORD n = GetModuleFileNameA(nullptr, dir, (DWORD)sizeof(dir));
    if (n && n < sizeof(dir)) {
        char* slash = std::strrchr(dir, '\\');
        if (!slash) slash = std::strrchr(dir, '/');
        if (slash) *(slash + 1) = 0;
        else std::snprintf(dir, sizeof(dir), ".\\");
    } else {
        std::snprintf(dir, sizeof(dir), ".\\");
    }
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
        std::fprintf(stderr, "[ps3rt] log file: %s\n", full);
    } else {
        std::fprintf(stderr, "[ps3rt] warning: could not create log at %s\n", full);
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
