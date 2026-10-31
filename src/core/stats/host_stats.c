#include "host_stats.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif

static uint64_t g_peak_ws;
static uint32_t g_peak_cpu_pm;
static uint64_t g_peak_io_r, g_peak_io_w;
static uint64_t g_presents;
static uint64_t g_last_cpu_time;
static uint64_t g_last_wall_ms;
static int g_inited;

#if defined(_WIN32)
static uint64_t filetime_to_u64(FILETIME ft) {
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}
#endif

void host_stats_init(void) {
    g_peak_ws = 0;
    g_peak_cpu_pm = 0;
    g_peak_io_r = g_peak_io_w = 0;
    g_presents = 0;
    g_last_cpu_time = 0;
    g_last_wall_ms = 0;
    g_inited = 1;
}

void host_stats_note_present(void) { ++g_presents; }
uint64_t host_stats_present_frames(void) { return g_presents; }
uint64_t host_stats_peak_working_set(void) { return g_peak_ws; }
uint32_t host_stats_peak_cpu_permille(void) { return g_peak_cpu_pm; }
uint64_t host_stats_peak_io_read(void) { return g_peak_io_r; }
uint64_t host_stats_peak_io_write(void) { return g_peak_io_w; }

void host_stats_sample(void) {
#if defined(_WIN32)
    HANDLE proc = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS_EX pmc;
    memset(&pmc, 0, sizeof(pmc));
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(proc, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        if (pmc.WorkingSetSize > g_peak_ws) g_peak_ws = pmc.WorkingSetSize;
        if (pmc.PrivateUsage > g_peak_ws) g_peak_ws = pmc.PrivateUsage;
    }
    IO_COUNTERS io;
    if (GetProcessIoCounters(proc, &io)) {
        if (io.ReadTransferCount > g_peak_io_r) g_peak_io_r = io.ReadTransferCount;
        if (io.WriteTransferCount > g_peak_io_w) g_peak_io_w = io.WriteTransferCount;
    }
    FILETIME cr, ex, kernel, user;
    if (GetProcessTimes(proc, &cr, &ex, &kernel, &user)) {
        uint64_t cpu = filetime_to_u64(kernel) + filetime_to_u64(user);
        uint64_t now = GetTickCount64();
        if (g_last_wall_ms && now > g_last_wall_ms + 200) {
            uint64_t dcpu = cpu - g_last_cpu_time;
            uint64_t dms = now - g_last_wall_ms;
            /* 100ns units: dcpu/10000 = ms of CPU time */
            uint64_t cpu_ms = dcpu / 10000ull;
            uint32_t pm = 0;
            if (dms > 0) {
                pm = (uint32_t)((cpu_ms * 1000ull) / dms);
                if (pm > 1000) pm = 1000;
            }
            if (pm > g_peak_cpu_pm) g_peak_cpu_pm = pm;
        }
        g_last_cpu_time = cpu;
        g_last_wall_ms = now;
    }
#else
    (void)0;
#endif
    if (!g_inited) host_stats_init();
}

void host_stats_write_log_section(void) {
    host_stats_sample();
    /* Written by caller into log file via fprintf, or we use stderr + expect hle_log to expose FILE*
       This function only formats through a weak extern fprintf helper. */
    extern void ps3rt_log(const char* fmt, ...);
    double mb = (double)g_peak_ws / (1024.0 * 1024.0);
    double rd = (double)g_peak_io_r / (1024.0 * 1024.0);
    double wr = (double)g_peak_io_w / (1024.0 * 1024.0);
    ps3rt_log("\n");
    ps3rt_log("======== RESOURCE PEAKS (game process) ========\n");
    ps3rt_log("CPU peak (approx): %u.%u%%\n", g_peak_cpu_pm / 10, g_peak_cpu_pm % 10);
    ps3rt_log("RAM peak (working set / private): %.1f MB\n", mb);
    ps3rt_log("Disk IO peak cumulative read:  %.1f MB\n", rd);
    ps3rt_log("Disk IO peak cumulative write: %.1f MB\n", wr);
    ps3rt_log("Present frames submitted: %llu\n", (unsigned long long)g_presents);
    ps3rt_log("GPU: Vulkan 1.2 light path (no heavy GPU counter API; frames=%llu)\n",
        (unsigned long long)g_presents);
    ps3rt_log("================================================\n");
}
