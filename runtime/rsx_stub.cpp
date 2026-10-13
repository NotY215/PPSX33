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
    uint64_t flip_count = 0;
    uint32_t flip_label = 0;
    bool flip_pending = false;
    int draws = 0;
    int clears = 0;
    int methods_seen = 0;
    int known_methods = 0;
    int unknown_methods = 0;
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t clear_color = 0x101828; // dark slate (Phase XIV)
};

static RsxState g_rsx;
static std::mutex g_rsx_mu;
static int g_backend = 1;
static HWND g_hwnd = nullptr;
static bool g_user_closed = false;
static bool g_resize_pending = false;
static UINT g_resize_w = 1280, g_resize_h = 720;
#if defined(_WIN32)
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
    g_hwnd = CreateWindowExW(0, L"PPSX33_RSX", L"PPSX33", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, (int)w, (int)h, nullptr, nullptr, wc.hInstance, nullptr);
    return g_hwnd != nullptr;
}

static void rsx_host_init(uint32_t w, uint32_t h) {
    if (!rsx_create_window(w, h)) return;
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = w; sd.BufferDesc.Height = h;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev11, &fl, &g_ctx11);
    if (FAILED(hr)) {
        ps3rt_log("[rsx] D3D11 create failed\n");
        return;
    }
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
    if (back) {
        g_dev11->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }
    g_rsx.width = w; g_rsx.height = h;
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

static void rsx_process_fifo(uint8_t* mem) {
    if (!mem || !g_rsx.fifo_addr || !g_rsx.fifo_size) return;
    uint32_t put = g_rsx.put % g_rsx.fifo_size;
    uint32_t get = g_rsx.get % g_rsx.fifo_size;
    int steps = 0;
    while (get != put && steps < 4096) {
        uint64_t fa = g_rsx.fifo_addr + get;
        if (fa + 8 > 512ull * 1024 * 1024) break;
        uint32_t hdr = (mem[fa] << 24) | (mem[fa+1] << 16) | (mem[fa+2] << 8) | mem[fa+3];
        uint32_t arg = (mem[fa+4] << 24) | (mem[fa+5] << 16) | (mem[fa+6] << 8) | mem[fa+7];
        uint32_t method = hdr & 0x1FFC;
        ++g_rsx.methods_seen;
        ++g_rsx.known_methods;
        if (method == 0x01D8 || method == 0x1D8) { ++g_rsx.clears; g_rsx.clear_color = arg; }
        else if (method == 0x0208 || method == 0x020C || method == 0x0A00 || method == 0x0A04) {}
        else if (method == 0x1800) { ++g_rsx.draws; }
        else { ++g_rsx.unknown_methods; --g_rsx.known_methods; }
        get = (get + 8) % g_rsx.fifo_size;
        ++steps;
    }
    g_rsx.get = get;
}

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
    if (!g_hwnd) rsx_host_init(1280, 720);
    ps3rt_log("[rsx] init fifo=0x%llx size=0x%x backend=%d\n", (unsigned long long)fifo_addr, g_rsx.fifo_size, g_backend);
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
    if (ps3rt_should_exit()) { ps3rt_force_exit(); return; }
    ++g_rsx.flip_count;
    g_rsx.flip_pending = false;
    rsx_host_present_clear();
    if (g_rsx.flip_count <= 32 || (g_rsx.flip_count & 0xFF) == 1) {
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
