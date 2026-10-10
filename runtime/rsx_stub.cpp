// Phase 5+: RSX / GCM FIFO decode + host present (D3D10 and D3D11).
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <cstdlib>

enum Ps3GfxBackend { PS3_GFX_NONE = 0, PS3_GFX_D3D10 = 1, PS3_GFX_D3D11 = 2, PS3_GFX_VULKAN = 3 };

namespace {

std::mutex g_rsx_mu;
int g_backend = PS3_GFX_D3D11;

struct RsxState {
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t pitch = 1280 * 4;
    uint64_t color_offset[8] = {};
    uint64_t depth_offset = 0;
    uint32_t color_format = 0x08;
    uint32_t depth_format = 0;
    uint32_t clip_w = 1280;
    uint32_t clip_h = 720;
    uint64_t fifo_addr = 0;
    uint32_t fifo_size = 0;
    uint32_t put = 0;
    uint32_t get = 0;
    uint64_t flip_count = 0;
    uint32_t flip_label = 0;
    bool flip_pending = false;
    uint32_t clear_color = 0;
    uint32_t clear_mask = 0;
    uint32_t viewport_x = 0, viewport_y = 0, viewport_w = 1280, viewport_h = 720;
    uint32_t scissor_x = 0, scissor_y = 0, scissor_w = 1280, scissor_h = 720;
    uint64_t label_addr = 0;
    int methods_seen = 0;
    int unknown_methods = 0;
    int draws = 0;
};

RsxState g_rsx;

enum {
    NV406E_SET_REFERENCE = 0x0050,
    NV406E_SEMAPHORE_OFFSET = 0x0064,
    NV406E_SEMAPHORE_RELEASE = 0x006C,
    NV4097_NO_OPERATION = 0x0100,
    NV4097_NOTIFY = 0x0104,
    NV4097_WAIT_FOR_IDLE = 0x0110,
    NV4097_PM_TRIGGER = 0x0140,
    NV4097_SET_SURFACE_FORMAT = 0x0200,
    NV4097_SET_SURFACE_PITCH_A = 0x0204,
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
    method &= 0x1FFC;
    switch (method) {
    case NV4097_NO_OPERATION: case NV4097_NOTIFY: case NV4097_WAIT_FOR_IDLE:
    case NV4097_PM_TRIGGER: case NV406E_SET_REFERENCE: break;
    case NV4097_SET_SURFACE_COLOR_AOFFSET: g_rsx.color_offset[0] = arg; break;
    case NV4097_SET_SURFACE_ZETA_OFFSET: g_rsx.depth_offset = arg; break;
    case NV4097_SET_SURFACE_PITCH_A: g_rsx.pitch = arg & 0xFFFF; break;
    case NV4097_SET_SURFACE_FORMAT:
        g_rsx.color_format = arg & 0x1F;
        g_rsx.depth_format = (arg >> 5) & 0x7;
        break;
    case NV4097_SET_SURFACE_CLIP_HORIZONTAL: g_rsx.clip_w = (arg >> 16) & 0xFFFF; break;
    case NV4097_SET_SURFACE_CLIP_VERTICAL:
        g_rsx.clip_h = (arg >> 16) & 0xFFFF;
        if (g_rsx.clip_h) g_rsx.height = g_rsx.clip_h;
        if (g_rsx.clip_w) g_rsx.width = g_rsx.clip_w;
        break;
    case NV4097_SET_VIEWPORT_HORIZONTAL:
        g_rsx.viewport_x = arg & 0xFFFF; g_rsx.viewport_w = (arg >> 16) & 0xFFFF; break;
    case NV4097_SET_VIEWPORT_VERTICAL:
        g_rsx.viewport_y = arg & 0xFFFF; g_rsx.viewport_h = (arg >> 16) & 0xFFFF; break;
    case NV4097_SET_SCISSOR_HORIZONTAL:
        g_rsx.scissor_x = arg & 0xFFFF; g_rsx.scissor_w = (arg >> 16) & 0xFFFF; break;
    case NV4097_SET_SCISSOR_VERTICAL:
        g_rsx.scissor_y = arg & 0xFFFF; g_rsx.scissor_h = (arg >> 16) & 0xFFFF; break;
    case NV4097_CLEAR_SURFACE: g_rsx.clear_mask = arg; break;
    case NV4097_SET_BEGIN_END: break;
    case NV4097_DRAW_ARRAYS: case NV4097_DRAW_INDEX_ARRAY: ++g_rsx.draws; break;
    case NV406E_SEMAPHORE_OFFSET: g_rsx.label_addr = arg; break;
    case NV406E_SEMAPHORE_RELEASE: g_rsx.flip_label = arg; break;
    default: ++g_rsx.unknown_methods; break;
    }
}

void rsx_process_fifo(uint8_t* mem) {
    if (!mem || !g_rsx.fifo_addr || g_rsx.put == g_rsx.get) return;
    uint32_t get = g_rsx.get;
    uint32_t put = g_rsx.put;
    uint32_t size = g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000;
    int steps = 0;
    while (get != put && steps < 65536) {
        uint64_t addr = g_rsx.fifo_addr + (get % size);
        uint32_t w = 0;
        std::memcpy(&w, mem + addr, 4);
        w = (w >> 24) | ((w >> 8) & 0xFF00) | ((w << 8) & 0xFF0000) | (w << 24);
        get = (get + 4) % size;
        ++steps;
        if ((w & 0xE0000000u) == 0) {
            uint32_t method = (w >> 2) & 0x7FF;
            uint32_t count = (w >> 18) & 0x7FF;
            for (uint32_t i = 0; i < count && get != put; ++i) {
                uint64_t da = g_rsx.fifo_addr + (get % size);
                uint32_t arg = 0;
                std::memcpy(&arg, mem + da, 4);
                arg = (arg >> 24) | ((arg >> 8) & 0xFF00) | ((arg << 8) & 0xFF0000) | (arg << 24);
                get = (get + 4) % size;
                rsx_apply_method(method + i * 4, arg);
            }
        }
    }
    g_rsx.get = get;
}

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3d10.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "dxgi.lib")

// Shared window
HWND g_hwnd = nullptr;
bool g_host_ready = false;

// D3D11 path
ID3D11Device*           g_dev11 = nullptr;
ID3D11DeviceContext*    g_ctx11 = nullptr;
IDXGISwapChain*         g_sc11  = nullptr;
ID3D11RenderTargetView* g_rtv11 = nullptr;

// D3D10 path
ID3D10Device*           g_dev10 = nullptr;
IDXGISwapChain*         g_sc10  = nullptr;
ID3D10RenderTargetView* g_rtv10 = nullptr;

LRESULT CALLBACK RsxWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

bool rsx_create_window(uint32_t w, uint32_t h) {
    if (g_hwnd) return true;
    WNDCLASSW wc = {};
    wc.lpfnWndProc = RsxWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PPSX33_RSX";
    RegisterClassW(&wc);
    g_hwnd = CreateWindowExW(0, L"PPSX33_RSX", L"PPSX33 RSX",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        (int)w + 16, (int)h + 39, nullptr, nullptr, wc.hInstance, nullptr);
    return g_hwnd != nullptr;
}

bool rsx_host_init_d3d11(uint32_t w, uint32_t h) {
    if (!rsx_create_window(w, h)) return false;
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = w;
    sd.BufferDesc.Height = h;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_sc11, &g_dev11, &fl, &g_ctx11);
    if (FAILED(hr)) {
        std::fprintf(stderr, "[rsx] D3D11 create failed hr=0x%08X\n", (unsigned)hr);
        return false;
    }
    ID3D11Texture2D* bb = nullptr;
    g_sc11->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (bb) {
        g_dev11->CreateRenderTargetView(bb, nullptr, &g_rtv11);
        bb->Release();
    }
    g_host_ready = true;
    std::fprintf(stderr, "[rsx] D3D11 host present ready %ux%u\n", w, h);
    return true;
}

bool rsx_host_init_d3d10(uint32_t w, uint32_t h) {
    if (!rsx_create_window(w, h)) return false;

    IDXGIFactory* factory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(hr) || !factory) {
        std::fprintf(stderr, "[rsx] DXGI factory failed hr=0x%08X\n", (unsigned)hr);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = w;
    sd.BufferDesc.Height = h;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    hr = D3D10CreateDeviceAndSwapChain(
        nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0,
        D3D10_SDK_VERSION, &sd, &g_sc10, &g_dev10);
    factory->Release();
    if (FAILED(hr)) {
        std::fprintf(stderr, "[rsx] D3D10 create failed hr=0x%08X\n", (unsigned)hr);
        return false;
    }
    ID3D10Texture2D* bb = nullptr;
    g_sc10->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&bb);
    if (bb) {
        g_dev10->CreateRenderTargetView(bb, nullptr, &g_rtv10);
        bb->Release();
    }
    g_host_ready = true;
    std::fprintf(stderr, "[rsx] D3D10 host present ready %ux%u\n", w, h);
    return true;
}

bool rsx_host_init(uint32_t w, uint32_t h) {
    if (g_host_ready) return true;
    // Honour env / selected backend
    const char* env = std::getenv("PS3RT_GFX");
    if (env) {
        if (std::strcmp(env, "D3D10") == 0) g_backend = PS3_GFX_D3D10;
        else if (std::strcmp(env, "D3D11") == 0) g_backend = PS3_GFX_D3D11;
        else if (std::strcmp(env, "Vulkan") == 0) g_backend = PS3_GFX_VULKAN;
    }
    if (g_backend == PS3_GFX_D3D10) {
        if (rsx_host_init_d3d10(w, h)) return true;
        std::fprintf(stderr, "[rsx] D3D10 failed — falling back to D3D11\n");
        return rsx_host_init_d3d11(w, h);
    }
    if (g_backend == PS3_GFX_D3D11) {
        if (rsx_host_init_d3d11(w, h)) return true;
        std::fprintf(stderr, "[rsx] D3D11 failed — trying D3D10\n");
        return rsx_host_init_d3d10(w, h);
    }
    // Vulkan not yet: try D3D11 then D3D10
    if (rsx_host_init_d3d11(w, h)) return true;
    return rsx_host_init_d3d10(w, h);
}

void rsx_host_present_clear() {
    if (!g_host_ready) return;
    float clear[4] = {
        ((g_rsx.clear_color >> 16) & 0xFF) / 255.f,
        ((g_rsx.clear_color >> 8) & 0xFF) / 255.f,
        (g_rsx.clear_color & 0xFF) / 255.f,
        ((g_rsx.clear_color >> 24) & 0xFF) / 255.f
    };
    if (!g_rsx.clear_mask) {
        clear[0] = 0.05f; clear[1] = 0.08f; clear[2] = 0.15f; clear[3] = 1.f;
    }

    if (g_ctx11 && g_rtv11 && g_sc11) {
        g_ctx11->OMSetRenderTargets(1, &g_rtv11, nullptr);
        g_ctx11->ClearRenderTargetView(g_rtv11, clear);
        g_sc11->Present(0, 0);
    } else if (g_dev10 && g_rtv10 && g_sc10) {
        g_dev10->OMSetRenderTargets(1, &g_rtv10, nullptr);
        g_dev10->ClearRenderTargetView(g_rtv10, clear);
        g_sc10->Present(0, 0);
    }

    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
#else
bool rsx_host_init(uint32_t, uint32_t) { return false; }
void rsx_host_present_clear() {}
#endif

} // namespace

PS3RT_API void ps3rt_set_graphics_backend(int backend) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    g_backend = backend;
}

PS3RT_API int ps3rt_get_graphics_backend(void) { return g_backend; }

PS3RT_API void ps3rt_rsx_init(uint64_t fifo_addr, uint32_t fifo_size) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    g_rsx.fifo_addr = fifo_addr;
    g_rsx.fifo_size = fifo_size;
    g_rsx.put = g_rsx.get = 0;
    if (g_backend == PS3_GFX_D3D11 || g_backend == PS3_GFX_D3D10 || g_backend == PS3_GFX_VULKAN)
        rsx_host_init(g_rsx.width, g_rsx.height);
    std::fprintf(stderr, "[rsx] init fifo=0x%llx size=0x%x backend=%d\n",
        (unsigned long long)fifo_addr, fifo_size, g_backend);
}

PS3RT_API void ps3rt_rsx_set_put(uint32_t put) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    g_rsx.put = put;
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
    ++g_rsx.flip_count;
    g_rsx.flip_pending = false;
    rsx_host_present_clear();
    if ((g_rsx.flip_count & 0x3F) == 1) {
        std::fprintf(stderr, "[rsx] flip #%llu draws=%d methods=%d unknown=%d %ux%u\n",
            (unsigned long long)g_rsx.flip_count, g_rsx.draws,
            g_rsx.methods_seen, g_rsx.unknown_methods, g_rsx.width, g_rsx.height);
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
