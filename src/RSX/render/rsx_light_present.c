#include "rsx_light_present.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/* ~8 Hz clear present — PS3 titles target 30/60; host assist stays light on weak GPUs */
static const uint32_t k_interval_ms = 125;
static uint64_t g_last_ms;

uint32_t rsx_light_present_interval_ms(void) { return k_interval_ms; }

int rsx_light_present_allow(void) {
#if defined(_WIN32)
    uint64_t now = GetTickCount64();
    if (!g_last_ms || now - g_last_ms >= k_interval_ms) return 1;
    return 0;
#else
    return 1;
#endif
}

void rsx_light_present_note(void) {
#if defined(_WIN32)
    g_last_ms = GetTickCount64();
#endif
}
