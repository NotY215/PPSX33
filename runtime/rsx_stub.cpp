// Phase 5: RSX / GCM core (control path + FIFO method decode).
// Host GPU backends (D3D11/Vulkan) still pending; this records state and flips.
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <mutex>

enum Ps3GfxBackend { PS3_GFX_NONE = 0, PS3_GFX_D3D10 = 1, PS3_GFX_D3D11 = 2, PS3_GFX_VULKAN = 3 };

namespace {

std::mutex g_rsx_mu;
int g_backend = PS3_GFX_D3D11;

struct RsxState {
    // Display
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t pitch = 1280 * 4;
    uint64_t color_offset[8] = {};
    uint64_t depth_offset = 0;
    uint32_t color_format = 0x08; // A8R8G8B8-ish
    uint32_t depth_format = 0;
    uint32_t clip_w = 1280;
    uint32_t clip_h = 720;

    // FIFO / put-get
    uint64_t fifo_addr = 0;
    uint32_t fifo_size = 0;
    uint32_t put = 0;
    uint32_t get = 0;

    // Frame
    uint64_t flip_count = 0;
    uint32_t flip_label = 0;
    bool flip_pending = false;

    // Clear
    uint32_t clear_color = 0;
    uint32_t clear_mask = 0;

    // Viewport / scissor (raw method values)
    uint32_t viewport_x = 0, viewport_y = 0, viewport_w = 1280, viewport_h = 720;
    uint32_t scissor_x = 0, scissor_y = 0, scissor_w = 1280, scissor_h = 720;

    // Semaphore
    uint64_t label_addr = 0;

    int methods_seen = 0;
    int unknown_methods = 0;
};

RsxState g_rsx;

// NV47-style method numbers (subset used by cellGcm / games)
enum {
    NV406E_SET_REFERENCE = 0x0050,
    NV406E_SET_CONTEXT_DMA_SEMAPHORE = 0x0060,
    NV406E_SEMAPHORE_OFFSET = 0x0064,
    NV406E_SEMAPHORE_ACQUIRE = 0x0068,
    NV406E_SEMAPHORE_RELEASE = 0x006C,

    NV4097_SET_OBJECT = 0x0000,
    NV4097_NO_OPERATION = 0x0100,
    NV4097_NOTIFY = 0x0104,
    NV4097_WAIT_FOR_IDLE = 0x0110,
    NV4097_PM_TRIGGER = 0x0140,
    NV4097_SET_CONTEXT_DMA_COLOR_A = 0x0194,
    NV4097_SET_CONTEXT_DMA_COLOR_B = 0x0184,
    NV4097_SET_CONTEXT_DMA_ZETA = 0x0198,
    NV4097_SET_SURFACE_FORMAT = 0x0200,
    NV4097_SET_SURFACE_PITCH_A = 0x0204,
    NV4097_SET_SURFACE_PITCH_Z = 0x020C,
    NV4097_SET_SURFACE_COLOR_AOFFSET = 0x0210,
    NV4097_SET_SURFACE_ZETA_OFFSET = 0x021C,
    NV4097_SET_SURFACE_CLIP_HORIZONTAL = 0x0208,
    NV4097_SET_SURFACE_CLIP_VERTICAL = 0x0214,
    NV4097_SET_VIEWPORT_HORIZONTAL = 0x0A00,
    NV4097_SET_VIEWPORT_VERTICAL = 0x0A04,
    NV4097_SET_SCISSOR_HORIZONTAL = 0x08C0,
    NV4097_SET_SCISSOR_VERTICAL = 0x08C4,
    NV4097_CLEAR_SURFACE = 0x01D8,
    NV4097_SET_BEGIN_END = 0x1808,
    NV4097_DRAW_ARRAYS = 0x1814,
    NV4097_DRAW_INDEX_ARRAY = 0x181C,
};

void rsx_apply_method(uint32_t method, uint32_t arg) {
    ++g_rsx.methods_seen;
    method &= 0x1FFC; // dword aligned method field

    switch (method) {
    case NV4097_NO_OPERATION:
    case NV4097_NOTIFY:
    case NV4097_WAIT_FOR_IDLE:
    case NV4097_PM_TRIGGER:
    case NV406E_SET_REFERENCE:
        break;

    case NV4097_SET_SURFACE_COLOR_AOFFSET:
        g_rsx.color_offset[0] = arg;
        break;
    case NV4097_SET_SURFACE_ZETA_OFFSET:
        g_rsx.depth_offset = arg;
        break;
    case NV4097_SET_SURFACE_PITCH_A:
        g_rsx.pitch = arg & 0xFFFF;
        break;
    case NV4097_SET_SURFACE_FORMAT:
        g_rsx.color_format = arg & 0x1F;
        g_rsx.depth_format = (arg >> 5) & 0x7;
        break;
    case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
        g_rsx.clip_w = (arg >> 16) & 0xFFFF;
        break;
    case NV4097_SET_SURFACE_CLIP_VERTICAL:
        g_rsx.clip_h = (arg >> 16) & 0xFFFF;
        g_rsx.height = g_rsx.clip_h ? g_rsx.clip_h : g_rsx.height;
        g_rsx.width = g_rsx.clip_w ? g_rsx.clip_w : g_rsx.width;
        break;
    case NV4097_SET_VIEWPORT_HORIZONTAL:
        g_rsx.viewport_x = arg & 0xFFFF;
        g_rsx.viewport_w = (arg >> 16) & 0xFFFF;
        break;
    case NV4097_SET_VIEWPORT_VERTICAL:
        g_rsx.viewport_y = arg & 0xFFFF;
        g_rsx.viewport_h = (arg >> 16) & 0xFFFF;
        break;
    case NV4097_SET_SCISSOR_HORIZONTAL:
        g_rsx.scissor_x = arg & 0xFFFF;
        g_rsx.scissor_w = (arg >> 16) & 0xFFFF;
        break;
    case NV4097_SET_SCISSOR_VERTICAL:
        g_rsx.scissor_y = arg & 0xFFFF;
        g_rsx.scissor_h = (arg >> 16) & 0xFFFF;
        break;
    case NV4097_CLEAR_SURFACE:
        g_rsx.clear_mask = arg;
        break;
    case NV4097_SET_BEGIN_END:
        // 0 = end, non-zero = begin prim type
        break;
    case NV4097_DRAW_ARRAYS:
    case NV4097_DRAW_INDEX_ARRAY:
        // Recorded; host draw later
        break;
    case NV406E_SEMAPHORE_OFFSET:
        g_rsx.label_addr = arg;
        break;
    case NV406E_SEMAPHORE_RELEASE:
        g_rsx.flip_label = arg;
        break;
    default:
        ++g_rsx.unknown_methods;
        break;
    }
}

// Walk FIFO from get to put. Words are BE in guest memory when used from PPU.
void rsx_process_fifo(uint8_t* mem, uint64_t guest_size) {
    if (!mem || !g_rsx.fifo_addr || g_rsx.fifo_size == 0) return;
    uint32_t get = g_rsx.get;
    uint32_t put = g_rsx.put;
    if (get == put) return;

    // Limit work per call
    int words = 0;
    while (get != put && words < 0x10000) {
        uint64_t addr = g_rsx.fifo_addr + (get & (g_rsx.fifo_size - 1));
        if (addr + 4 > guest_size) break;
        uint32_t w;
        std::memcpy(&w, mem + addr, 4);
        w = bs32(w);
        get = (get + 4) & (g_rsx.fifo_size - 1);
        ++words;

        // Method header: count in high bits, method in low
        // Cell GCM: (count << 18) | (method & 0x1FFC) style variants exist
        uint32_t method = w & 0x1FFC;
        uint32_t count = (w >> 18) & 0x7FF;
        if (count == 0) count = 1;

        for (uint32_t i = 0; i < count && get != put; ++i) {
            uint64_t a2 = g_rsx.fifo_addr + (get & (g_rsx.fifo_size - 1));
            if (a2 + 4 > guest_size) break;
            uint32_t arg;
            std::memcpy(&arg, mem + a2, 4);
            arg = bs32(arg);
            get = (get + 4) & (g_rsx.fifo_size - 1);
            ++words;
            rsx_apply_method(method + i * 4, arg);
        }
    }
    g_rsx.get = get;
}

} // namespace

PS3RT_API void ps3rt_set_graphics_backend(int backend) { g_backend = backend; }
PS3RT_API int  ps3rt_get_graphics_backend(void) { return g_backend; }

PS3RT_API void ps3rt_rsx_init(uint64_t fifo_addr, uint32_t fifo_size) {
    std::lock_guard<std::mutex> lock(g_rsx_mu);
    g_rsx = RsxState{};
    g_rsx.fifo_addr = fifo_addr;
    g_rsx.fifo_size = fifo_size ? fifo_size : 0x100000;
    std::fprintf(stderr, "[rsx] init fifo=0x%llx size=0x%x backend=%d\n",
        (unsigned long long)fifo_addr, g_rsx.fifo_size, g_backend);
}

PS3RT_API void ps3rt_rsx_set_put(uint32_t put) {
    std::lock_guard<std::mutex> lock(g_rsx_mu);
    g_rsx.put = put;
}

PS3RT_API uint32_t ps3rt_rsx_get_get(void) {
    std::lock_guard<std::mutex> lock(g_rsx_mu);
    return g_rsx.get;
}

PS3RT_API void ps3rt_rsx_flush(uint8_t* mem) {
    std::lock_guard<std::mutex> lock(g_rsx_mu);
    rsx_process_fifo(mem, 1ull << 32);
}

PS3RT_API void ps3rt_rsx_flip(uint32_t buf_id) {
    std::lock_guard<std::mutex> lock(g_rsx_mu);
    g_rsx.flip_pending = false;
    ++g_rsx.flip_count;
    if (g_rsx.flip_count <= 5 || (g_rsx.flip_count % 60) == 0) {
        std::fprintf(stderr,
            "[rsx] flip #%llu buf=%u %ux%u methods=%d unknown=%d backend=%d\n",
            (unsigned long long)g_rsx.flip_count, buf_id,
            g_rsx.width, g_rsx.height,
            g_rsx.methods_seen, g_rsx.unknown_methods, g_backend);
    }
}

PS3RT_API uint64_t ps3rt_rsx_flip_count(void) {
    return g_rsx.flip_count;
}

// Called from syscall HLE for cellGcm* style entry points
PS3RT_API int ps3rt_rsx_cell_gcm_syscall(PPUContext* c, uint64_t nid_or_num) {
    // Lightweight HLE: succeed and advance state for common patterns.
    // nid_or_num currently uses LV2/syscall numbers or hashed IDs from callers.
    (void)nid_or_num;
    if (!c) return -1;

    // Heuristic: r3 often holds context / fifo put
    // cellGcmSetFlip / Flip: treat as flip
    ps3rt_rsx_flush(c->mem);
    ps3rt_rsx_flip(0);
    c->gpr[3] = 0; // CELL_OK
    return 0;
}
