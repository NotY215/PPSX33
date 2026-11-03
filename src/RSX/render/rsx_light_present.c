#include "rsx_light_present.h"
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static const uint32_t k_interval_ms = 33; /* ~30 Hz host assist */
static uint64_t g_last_ms;
static int s_count;

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
    ++s_count;
}

int rsx_light_present_count(void) { return s_count; }
