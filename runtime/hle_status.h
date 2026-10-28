#pragma once
// Module-level HLE status for diagnostics (not a claim of title compatibility).
typedef enum Ps3HleStatus {
    PS3_HLE_COMPLETE = 0,
    PS3_HLE_PARTIAL = 1,
    PS3_HLE_STUB = 2,
    PS3_HLE_OUT_OF_SCOPE = 3
} Ps3HleStatus;

typedef struct Ps3HleModuleInfo {
    const char* name;
    Ps3HleStatus status;
    const char* notes;
} Ps3HleModuleInfo;

static const Ps3HleModuleInfo k_ps3_hle_modules[] = {
    { "sys_ppu_thread/mutex/cond", PS3_HLE_PARTIAL, "subset" },
    { "sys_memory/mmapper", PS3_HLE_PARTIAL, "subset" },
    { "cellFs", PS3_HLE_PARTIAL, "path map + host I/O" },
    { "cellPad", PS3_HLE_PARTIAL, "XInput/host keys" },
    { "cellAudio", PS3_HLE_PARTIAL, "host path" },
    { "cellGcmSys", PS3_HLE_PARTIAL, "control/FIFO kick" },
    { "cellSysutil/sysmodule", PS3_HLE_PARTIAL, "common NIDs" },
    { "cellSpurs", PS3_HLE_PARTIAL, "job assist" },
    { "RSX/FIFO", PS3_HLE_PARTIAL, "assist draws proven; title stream pending" },
    { "cellNet/NP", PS3_HLE_OUT_OF_SCOPE, "not a product goal" },
};
static const int k_ps3_hle_modules_count =
    (int)(sizeof(k_ps3_hle_modules) / sizeof(k_ps3_hle_modules[0]));
