// Phase H: hardware progress helper (linked into ps3rt.dll)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdint>

// These symbols live in ps3rt.cpp
extern uint8_t* g_mem;
extern size_t g_mem_size;
extern uint64_t g_gcm_control;
extern uint64_t g_gcm_labels;
extern uint32_t g_gcm_flip_status;
extern uint32_t g_gcm_frame_id;

static void wr32(uint64_t addr, uint32_t val) {
    if (!g_mem || addr + 4 > g_mem_size) return;
    g_mem[addr]   = (uint8_t)(val >> 24);
    g_mem[addr+1] = (uint8_t)(val >> 16);
    g_mem[addr+2] = (uint8_t)(val >> 8);
    g_mem[addr+3] = (uint8_t)val;
}
static uint32_t rd32(uint64_t addr) {
    if (!g_mem || addr + 4 > g_mem_size) return 0;
    return ((uint32_t)g_mem[addr] << 24) | ((uint32_t)g_mem[addr+1] << 16) |
           ((uint32_t)g_mem[addr+2] << 8) | (uint32_t)g_mem[addr+3];
}

PS3RT_API void ps3rt_progress_hw(void) {
    if (!g_mem) return;
    if (g_gcm_control && g_gcm_control + 12 <= g_mem_size) {
        uint32_t put = rd32(g_gcm_control + 0);
        uint32_t get = rd32(g_gcm_control + 8);
        if (put != get) {
            wr32(g_gcm_control + 8, put);
            ps3rt_rsx_set_put(put);
            ps3rt_rsx_flush(g_mem);
        }
        wr32(g_gcm_control + 4, put);
    }
    g_gcm_flip_status = 0;
    static uint32_t s_flip_div = 0;
    if ((++s_flip_div & 0xF) == 0) {
        ps3rt_rsx_flip(0);
        g_gcm_frame_id++;
    }
    if (g_gcm_labels && g_gcm_labels + 0x100 <= g_mem_size) {
        for (int i = 0; i < 16; ++i)
            wr32(g_gcm_labels + (uint64_t)i * 0x10, 0);
    }
}
