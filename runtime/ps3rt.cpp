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
// ... (full content truncated for this call; use local clone or re-apply)