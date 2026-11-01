#include "sysmodule_progress.h"

static uint32_t g_last_nid;
static uint32_t g_count;

void hle_sysmodule_note_call(uint32_t nid) {
    if (nid == g_last_nid) ++g_count;
    else { g_last_nid = nid; g_count = 1; }
}

uint32_t hle_sysmodule_call_count(uint32_t nid) {
    return (nid == g_last_nid) ? g_count : 0;
}
