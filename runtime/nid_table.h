// Minimal NID → module/function table for HLE dispatch (research)
#pragma once
#include <cstdint>

struct NidEntry {
    uint32_t nid;
    const char* module;
    const char* name;
    int kind; // 1=RSX/Gcm 2=cellFs 7=video ...
};

static const NidEntry kNidTable[] = {
    { 0x2137BB2A, "sysPrxForUser", "sys_time_get_system_time", 0 },
    { 0x2C847572, "sysPrxForUser", "sys_process_exit", 0 },
    { 0xA547ADDE, "cellGcmSys", "cellGcmGetFlipStatus", 1 },
    { 0xB2E761D4, "cellGcmSys", "cellGcmResetFlipStatus", 1 },
    { 0x718BF5F8, "cellFs", "cellFsOpen", 2 },
    { 0x4D5B9EF2, "cellFs", "cellFsRead", 2 },
    { 0xECDCF2AB, "cellFs", "cellFsWrite", 2 },
    { 0x2CB51F0F, "cellFs", "cellFsClose", 2 },
    { 0xA397D042, "cellFs", "cellFsLseek", 2 },
    { 0x7F13FC8C, "cellFs", "cellFsStat", 2 },
    { 0xEF3EFA34, "cellFs", "cellFsFstat", 2 },
    { 0xE558748D, "cellVideoOut", "cellVideoOutGetState", 7 },
};

static inline int prx_module_kind(const char* name) {
    if (!name) return 0;
    // Simple classification used by import stubs
    if (name[0] == 'c' || name[0] == 'C') {
        // cell*
        if (name[4] == 'F' || name[4] == 'f') return 2; // cellFs*
        if (name[4] == 'G' || name[4] == 'g') return 1; // cellGcm*
        if (name[4] == 'V' || name[4] == 'v') return 7; // cellVideo*
    }
    return 0;
}

static inline const NidEntry* nid_lookup(uint32_t nid) {
    for (const auto& e : kNidTable)
        if (e.nid == nid) return &e;
    return nullptr;
}
