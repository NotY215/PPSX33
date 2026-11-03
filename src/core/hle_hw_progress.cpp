// WORK: HW progress tick + guest-PUT probe.
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "rsx_fifo_drain.h"
#include "gcm_flip_label.h"
#include "rsx_frag_align.h"
#include "rsx_present_queue.h"
#include <cstdint>

extern uint8_t* g_mem;
extern size_t g_mem_size;
extern uint64_t g_gcm_control;
extern uint64_t g_gcm_labels;
extern uint32_t g_gcm_flip_status;
extern uint32_t g_gcm_frame_id;

static void wr32(uint64_t addr, uint32_t val) {
    if (!g_mem || addr + 4 > g_mem_size) return;
    g_mem[addr]     = (uint8_t)(val >> 24);
    g_mem[addr + 1] = (uint8_t)(val >> 16);
    g_mem[addr + 2] = (uint8_t)(val >> 8);
    g_mem[addr + 3] = (uint8_t)val;
}

static uint32_t rd32_be(uint64_t addr) {
    if (!g_mem || addr + 4 > g_mem_size) return 0;
    return ((uint32_t)g_mem[addr] << 24) | ((uint32_t)g_mem[addr + 1] << 16) |
           ((uint32_t)g_mem[addr + 2] << 8) | (uint32_t)g_mem[addr + 3];
}

static uint32_t rd32_le(uint64_t addr) {
    if (!g_mem || addr + 4 > g_mem_size) return 0;
    return (uint32_t)g_mem[addr] | ((uint32_t)g_mem[addr + 1] << 8) |
           ((uint32_t)g_mem[addr + 2] << 16) | ((uint32_t)g_mem[addr + 3] << 24);
}

// Prefer BE; if BE put/get both zero but LE shows activity, use LE (AIFIAR control endian).
static uint32_t rd_ctrl(uint64_t addr) {
    uint32_t be = rd32_be(addr);
    uint32_t le = rd32_le(addr);
    if (be != 0) return be;
    if (le != 0) {
        static int s_le = 0;
        if (s_le < 8) {
            ps3rt_log("[rsx] control LE put/get @0x%llx val=0x%x\n",
                      (unsigned long long)addr, le);
            ++s_le;
        }
        return le;
    }
    return be;
}

PS3RT_API void ps3rt_progress_hw(void) {
    if (!g_mem) return;

    if (g_gcm_control && g_gcm_control + 12 <= g_mem_size) {
        uint32_t put = rd_ctrl(g_gcm_control + 0);
        uint32_t ref = rd_ctrl(g_gcm_control + 4);
        uint32_t get = rd_ctrl(g_gcm_control + 8);
        if (get == put && ref != put)
            get = ref;

        static uint32_t s_last_put = 0;
        static int s_put_log = 0;
        if (put != s_last_put) {
            if (s_put_log < 32) {
                ps3rt_log("[rsx] guest-PUT put=0x%x get=0x%x ref=0x%x\n", put, get, ref);
                ++s_put_log;
            }
            s_last_put = put;
        }

        if (put != get) {
            ps3rt_rsx_set_put(put);
            ps3rt_rsx_flush(g_mem);
            uint32_t host_get = ps3rt_rsx_get_get();
            wr32(g_gcm_control + 8, host_get);
            /* Extra publish walk when fifo base unknown: coarse GET advance */
            uint32_t methods = 0, draws = 0;
            rsx_fifo_drain_publish(g_mem, (uint64_t)g_mem_size, g_gcm_control,
                                   0 /* base unknown */, 0x100000u, &methods, &draws);
        }
        wr32(g_gcm_control + 4, put);
    }


    /* Frag align orchestration: surface + fifo slice + pattern present + audio */
    {
        static rsx_frag_align_ctx s_ax;
        static int s_ax_init = 0;
        if (!s_ax_init) { rsx_frag_align_init(&s_ax); s_ax_init = 1; }
        s_ax.mem = g_mem;
        s_ax.mem_size = (uint64_t)g_mem_size;
        s_ax.ctrl_ea = g_gcm_control;
        s_ax.labels_ea = g_gcm_labels;
        s_ax.fifo_base = 0;
        s_ax.fifo_size = 0x100000u;
        rsx_frag_align_tick(&s_ax);
        if (rsx_frag_align_want_guest_present(&s_ax)) {
            rsx_present_req pr{};
            pr.color_offset = s_ax.present.color_offset;
            pr.pitch = s_ax.present.pitch;
            pr.width = s_ax.present.width;
            pr.height = s_ax.present.height;
            pr.flags = s_ax.present.flags;
            rsx_present_queue_push(&pr);
        }
    }
    g_gcm_flip_status = 0;
    gcm_set_flip_status(0);
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
