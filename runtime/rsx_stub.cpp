// Phase XV: RSX window + debounced resize; close never kills process
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "ps3rt_stage.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <cstdlib>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d11.h>
#include <d3d10.h>
#include <dxgi.h>
#endif

struct RsxState {
    uint64_t fifo_addr = 0;
    uint32_t fifo_size = 0;
    uint32_t put = 0;
    uint32_t get = 0;
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t pitch = 1280 * 4;
    uint32_t color_format = 0;
    uint32_t depth_format = 0;
    uint32_t color_offset[4] = {};
    uint32_t depth_offset = 0;
    uint32_t clip_w = 1280, clip_h = 720;
    uint32_t viewport_x = 0, viewport_y = 0, viewport_w = 1280, viewport_h = 720;
    uint32_t scissor_x = 0, scissor_y = 0, scissor_w = 1280, scissor_h = 720;
    uint32_t clear_mask = 0;
    uint32_t clear_color = 0xFF1a2a3a;
    uint64_t flip_count = 0;
    uint32_t flip_label = 0;
    bool flip_pending = false;
    bool in_begin = false;
    uint64_t label_addr = 0;
    int methods_seen = 0;
    int known_methods = 0;
    int unknown_methods = 0;
    int draws = 0;
    int clears = 0;
};
static RsxState g_rsx;
static std::mutex g_rsx_mu;
static int g_backend = 1;

enum {
    NV4097_NO_OPERATION = 0x0100,
    NV4097_NOTIFY = 0x0104,
    NV4097_WAIT_FOR_IDLE = 0x0110,
    NV4097_PM_TRIGGER = 0x0140,
    NV4097_SET_CONTEXT_DMA_COLOR_A = 0x0194,
    NV4097_SET_CONTEXT_DMA_ZETA = 0x0198,
    NV4097_SET_SURFACE_FORMAT = 0x0200,
    NV4097_SET_SURFACE_PITCH_A = 0x0204,
    NV4097_SET_SURFACE_CLIP_HORIZONTAL = 0x0208,
    NV4097_SET_SURFACE_CLIP_VERTICAL = 0x020C,
    NV4097_SET_SURFACE_COLOR_AOFFSET = 0x0210,
    NV4097_SET_SURFACE_ZETA_OFFSET = 0x0214,
    NV4097_SET_SURFACE_COLOR_BOFFSET = 0x0218,
    NV4097_CLEAR_SURFACE = 0x01D8,
    NV4097_SET_VIEWPORT_HORIZONTAL = 0x0A00,
    NV4097_SET_VIEWPORT_VERTICAL = 0x0A04,
    NV4097_SET_SCISSOR_HORIZONTAL = 0x08C0,
    NV4097_SET_SCISSOR_VERTICAL = 0x08C4,
    NV4097_SET_BEGIN_END = 0x1808,
    NV4097_DRAW_ARRAYS = 0x1814,
    NV4097_DRAW_INDEX_ARRAY = 0x181C,
    NV4097_INLINE_ARRAY = 0x1818,
    NV406E_SET_REFERENCE = 0x0050,
    NV406E_SEMAPHORE_OFFSET = 0x0060,
    NV406E_SEMAPHORE_ACQUIRE = 0x0064,
    NV406E_SEMAPHORE_RELEASE = 0x0068,
};

void rsx_apply_method(uint32_t method, uint32_t arg) {
    ++g_rsx.methods_seen;
    method &= 0x1FFC;
    bool known = true;
    switch (method) {
    case NV4097_NO_OPERATION: case NV4097_NOTIFY: case NV4097_WAIT_FOR_IDLE:
    case NV4097_PM_TRIGGER: case NV406E_SET_REFERENCE: break;
    case NV4097_SET_CONTEXT_DMA_COLOR_A: case NV4097_SET_CONTEXT_DMA_ZETA: break;
    case NV4097_SET_SURFACE_COLOR_AOFFSET: g_rsx.color_offset[0] = arg; break;
    case NV4097_SET_SURFACE_ZETA_OFFSET: g_rsx.depth_offset = arg; break;
    case NV4097_SET_SURFACE_COLOR_BOFFSET: g_rsx.color_offset[1] = arg; break;
    case NV4097_SET_SURFACE_PITCH_A: g_rsx.pitch = arg & 0xFFFF; break;
    case NV4097_SET_SURFACE_FORMAT:
        g_rsx.color_format = arg & 0x1F; g_rsx.depth_format = (arg >> 5) & 0x7; break;
    case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
        g_rsx.clip_w = (arg >> 16) & 0xFFFF; if (g_rsx.clip_w) g_rsx.width = g_rsx.clip_w; break;
    case NV4097_SET_SURFACE_CLIP_VERTICAL:
        g_rsx.clip_h = (arg >> 16) & 0xFFFF; if (g_rsx.clip_h) g_rsx.height = g_rsx.clip_h; break;
    case NV4097_SET_VIEWPORT_HORIZONTAL:
        g_rsx.viewport_x = arg & 0xFFFF; g_rsx.viewport_w = (arg >> 16) & 0xFFFF; break;
    case NV4097_SET_VIEWPORT_VERTICAL:
        g_rsx.viewport_y = arg & 0xFFFF; g_rsx.viewport_h = (arg >> 16) & 0xFFFF; break;
    case NV4097_SET_SCISSOR_HORIZONTAL:
        g_rsx.scissor_x = arg & 0xFFFF; g_rsx.scissor_w = (arg >> 16) & 0xFFFF; break;
    case NV4097_SET_SCISSOR_VERTICAL:
        g_rsx.scissor_y = arg & 0xFFFF; g_rsx.scissor_h = (arg >> 16) & 0xFFFF; break;
    case NV4097_CLEAR_SURFACE:
        g_rsx.clear_mask = arg;
        g_rsx.clear_color = 0xFF000000u
            | ((g_rsx.clears * 17u) & 0x3Fu) << 16
            | (0x2Au + ((g_rsx.clears * 3u) & 0x1Fu)) << 8
            | (0x3Au + ((g_rsx.clears * 5u) & 0x1Fu));
        ++g_rsx.clears; ++g_rsx.draws; break;
    case NV4097_SET_BEGIN_END: g_rsx.in_begin = (arg != 0); break;
    case NV4097_DRAW_ARRAYS: case NV4097_DRAW_INDEX_ARRAY: case NV4097_INLINE_ARRAY:
        ++g_rsx.draws; break;
    case NV406E_SEMAPHORE_OFFSET: g_rsx.label_addr = arg; break;
    case NV406E_SEMAPHORE_RELEASE: case NV406E_SEMAPHORE_ACQUIRE:
        g_rsx.flip_label = arg; break;
    default: known = false; ++g_rsx.unknown_methods; break;
    }
    if (known) ++g_rsx.known_methods;
}

void rsx_process_fifo(uint8_t* mem) {
    if (!mem || !g_rsx.fifo_addr) return;
    uint32_t size = g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000;
    uint32_t get = g_rsx.get % size;
    uint32_t put = g_rsx.put % size;
    if (get == put) return;
    int steps = 0;
    while (get != put && steps < 8192) {
        uint64_t addr = g_rsx.fifo_addr + get;
        if (addr + 4 > 512ull * 1024 * 1024) break;
        uint32_t hdr = ((uint32_t)mem[addr] << 24) | ((uint32_t)mem[addr+1] << 16) |
                       ((uint32_t)mem[addr+2] << 8) | (uint32_t)mem[addr+3];
        get = (get + 4) % size;
        uint32_t method = hdr & 0x1FFC;
        uint32_t count = (hdr >> 18) & 0x7FF;
        if (count == 0) count = 1;
        if (count > 0x400) count = 1;
        for (uint32_t i = 0; i < count && get != put; ++i) {
            uint64_t a2 = g_rsx.fifo_addr + get;
            if (a2 + 4 > 512ull * 1024 * 1024) break;
            uint32_t arg = ((uint32_t)mem[a2] << 24) | ((uint32_t)mem[a2+1] << 16) |
                           ((uint32_t)mem[a2+2] << 8) | (uint32_t)mem[a2+3];
            get = (get + 4) % size;
            rsx_apply_method(method + i * 4, arg);
        }
        ++steps;
    }
    g_rsx.get = get;
}

#if defined(_WIN32)
static HWND g_hwnd = nullptr;
static bool g_user_closed = false;
static bool g_resize_pending = false;
static UINT g_resize_w = 0, g_resize_h = 0;
static ID3D11Device* g_dev11 = nullptr;
static ID3D11DeviceContext* g_ctx11 = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;

static LRESULT CALLBACK RsxWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE || (m == WM_SYSCOMMAND && (w & 0xFFF0) == SC_CLOSE)) {
        g_user_closed = true;
        ps3rt_log("[rsx] window closed by user — exiting\n");
        ps3rt_request_exit();
        DestroyWindow(h);
        PostQuitMessage(0);
        ps3rt_force_exit();
        return 0;
    }
    if (m == WM_DESTROY) {
        if (h == g_hwnd) g_hwnd = nullptr;
        g_user_closed = true;
        ps3rt_request_exit();
        PostQuitMessage(0);
        ps3rt_force_exit();
        return 0;
    }
    if (m == WM_SYSCOMMAND && (w & 0xFFF0) == SC_CLOSE) {
        ShowWindow(h, SW_HIDE);
        g_user_closed = true;
        return 0;
    }
    if (m == WM_SIZE) {
        UINT nw = (UINT)LOWORD(l), nh = (UINT)HIWORD(l);
        if (nw >= 64 && nh >= 64) {
            g_resize_w = nw; g_resize_h = nh;
            g_resize_pending = true;
        }
        return 0;
    }
    if (m == WM_ENTERSIZEMOVE || m == WM_EXITSIZEMOVE) return 0;
    return DefWindowProcW(h, m, w, l);
}

static bool rsx_create_window(uint32_t w, uint32_t h) {
    if (g_hwnd) return true;
    WNDCLASSW wc{};
    wc.lpfnWndProc = RsxWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PPSX33_RSX";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    g_hwnd = CreateWindowExW(0, L"PPSX33_RSX", L"PPSX33 RSX", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, (int)w + 16, (int)h + 39,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) return false;
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    return true;
}

static void rsx_host_init(uint32_t w, uint32_t h) {
    if (!rsx_create_window(w, h)) return;
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = w;
    sd.BufferDesc.Height = h;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev11, nullptr, &g_ctx11);
    if (FAILED(hr)) {
        ps3rt_log("[rsx] D3D11 create failed hr=0x%08X\n", (unsigned)hr);
        return;
    }
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
    if (back) {
        g_dev11->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }
    ps3rt_log("[rsx] D3D11 host present ready %ux%u\n", w, h);
    PS3RT_STAGE("RSX ready end");
}

static void rsx_host_present_clear() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (ps3rt_should_exit() || g_user_closed) {
        ps3rt_log("[rsx] exit after message pump\n");
        ps3rt_force_exit();
        return;
    }
    if (!g_hwnd) return;
    if (!g_ctx11 || !g_swap) return;
    if (g_resize_pending && g_resize_w >= 64 && g_resize_h >= 64) {
        g_resize_pending = false;
        g_ctx11->OMSetRenderTargets(0, nullptr, nullptr);
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
        HRESULT hr = g_swap->ResizeBuffers(0, g_resize_w, g_resize_h, DXGI_FORMAT_UNKNOWN, 0);
        if (SUCCEEDED(hr)) {
            ID3D11Texture2D* back = nullptr;
            g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
            if (back) {
                g_dev11->CreateRenderTargetView(back, nullptr, &g_rtv);
                back->Release();
            }
            g_rsx.width = g_resize_w;
            g_rsx.height = g_resize_h;
        }
    }
    if (!g_rtv) return;
    float r = ((g_rsx.clear_color >> 16) & 0xFF) / 255.f;
    float g = ((g_rsx.clear_color >> 8) & 0xFF) / 255.f;
    float b = (g_rsx.clear_color & 0xFF) / 255.f;
    float col[4] = { r, g, b, 1.f };
    try {
        g_ctx11->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx11->ClearRenderTargetView(g_rtv, col);
        g_swap->Present(0, 0);
    } catch (...) {
        ps3rt_log("[rsx] present exception — switching headless\n");
        g_user_closed = true;
    }
}
#else
static void rsx_host_init(uint32_t, uint32_t) {}
static void rsx_host_present_clear() {}
#endif

PS3RT_API void ps3rt_set_graphics_backend(int backend) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    g_backend = backend;
}
PS3RT_API int ps3rt_get_graphics_backend(void) { return g_backend; }

PS3RT_API void ps3rt_rsx_init(uint64_t fifo_addr, uint32_t fifo_size) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    g_rsx.fifo_addr = fifo_addr;
    g_rsx.fifo_size = fifo_size ? fifo_size : 0x100000;
    g_rsx.put = g_rsx.get = 0;
    g_rsx.methods_seen = g_rsx.known_methods = g_rsx.unknown_methods = 0;
    g_rsx.draws = g_rsx.clears = 0;
    if (g_backend == 1 || g_backend == 2)
        rsx_host_init(g_rsx.width, g_rsx.height);
    ps3rt_log("[rsx] init fifo=0x%llx size=0x%x backend=%d\n",
        (unsigned long long)fifo_addr, g_rsx.fifo_size, g_backend);
}

PS3RT_API void ps3rt_rsx_set_put(uint32_t put) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    uint32_t size = g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000;
    g_rsx.put = put % size;
}

PS3RT_API uint32_t ps3rt_rsx_get_get(void) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    return g_rsx.get;
}

PS3RT_API void ps3rt_rsx_flush(uint8_t* mem) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    rsx_process_fifo(mem);
}

PS3RT_API void ps3rt_rsx_flip(uint32_t buf_id) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    (void)buf_id;
    if (ps3rt_should_exit()) { ps3rt_force_exit(); return; }
    ++g_rsx.flip_count;
    g_rsx.flip_pending = false;
    rsx_host_present_clear();
    if ((g_rsx.flip_count & 0xF) == 1) {
        ps3rt_log("[rsx] flip #%llu draws=%d clears=%d methods=%d known=%d unknown=%d %ux%u\n",
            (unsigned long long)g_rsx.flip_count, g_rsx.draws, g_rsx.clears,
            g_rsx.methods_seen, g_rsx.known_methods, g_rsx.unknown_methods,
            g_rsx.width, g_rsx.height);
    }
}

PS3RT_API uint64_t ps3rt_rsx_flip_count(void) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    return g_rsx.flip_count;
}

PS3RT_API int ps3rt_rsx_cell_gcm_syscall(PPUContext* c, uint64_t) {
    if (c) c->gpr[3] = 0;
    return 0;
}
