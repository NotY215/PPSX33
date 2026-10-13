// Phase XV: RSX + depth/stencil + fixed-function draw path
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
    int depth_clears = 0;
    int stencil_clears = 0;
    int methods_seen = 0;
    int known_methods = 0;
    int unknown_methods = 0;
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t clear_color = 0x101828;
    uint32_t clear_mask = 0xF1;
    float clear_depth = 1.0f;
    uint8_t clear_stencil = 0;
    bool depth_enable = true;
    bool depth_write = true;
    bool stencil_enable = false;
    // Phase XV fixed-function
    int draws_this_frame = 0;
    int shader_binds = 0;
    int tex_binds = 0;
    uint32_t viewport_w = 1280, viewport_h = 720;
    uint32_t scissor_x = 0, scissor_y = 0, scissor_w = 1280, scissor_h = 720;
    uint32_t prim_color = 0xC0A060;
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
static ID3D11Texture2D* g_depth_tex = nullptr;
static ID3D11DepthStencilView* g_dsv = nullptr;
static ID3D11DepthStencilState* g_dss = nullptr;

static void rsx_release_depth() {
    if (g_dss) { g_dss->Release(); g_dss = nullptr; }
    if (g_dsv) { g_dsv->Release(); g_dsv = nullptr; }
    if (g_depth_tex) { g_depth_tex->Release(); g_depth_tex = nullptr; }
}

static bool rsx_ensure_depth(uint32_t w, uint32_t h) {
    if (!g_dev11) return false;
    if (g_dsv && g_depth_tex) return true;
    rsx_release_depth();
    D3D11_TEXTURE2D_DESC td{};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (FAILED(g_dev11->CreateTexture2D(&td, nullptr, &g_depth_tex))) return false;
    D3D11_DEPTH_STENCIL_VIEW_DESC vd{};
    vd.Format = td.Format; vd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(g_dev11->CreateDepthStencilView(g_depth_tex, &vd, &g_dsv))) return false;
    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = TRUE; dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dd.StencilEnable = FALSE;
    g_dev11->CreateDepthStencilState(&dd, &g_dss);
    return g_dsv != nullptr;
}

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
        if (nw >= 64 && nh >= 64) { g_resize_w = nw; g_resize_h = nh; g_resize_pending = true; }
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
    if (FAILED(hr)) { ps3rt_log("[rsx] D3D11 create failed\n"); return; }
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
    if (back) { g_dev11->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
    g_rsx.width = w; g_rsx.height = h;
    rsx_ensure_depth(w, h);
    ps3rt_log("[rsx] D3D11 host present ready %ux%u (depth+stencil)\n", w, h);
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
    if (!g_hwnd || !g_ctx11 || !g_swap) return;
    if (g_resize_pending && g_resize_w >= 64 && g_resize_h >= 64) {
        g_resize_pending = false;
        g_ctx11->OMSetRenderTargets(0, nullptr, nullptr);
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
        HRESULT hr = g_swap->ResizeBuffers(0, g_resize_w, g_resize_h, DXGI_FORMAT_UNKNOWN, 0);
        if (SUCCEEDED(hr)) {
            ID3D11Texture2D* back = nullptr;
            g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
            if (back) { g_dev11->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
            g_rsx.width = g_resize_w; g_rsx.height = g_resize_h;
            rsx_release_depth();
            rsx_ensure_depth(g_rsx.width, g_rsx.height);
        }
    }
    if (!g_rtv) return;
    rsx_ensure_depth(g_rsx.width, g_rsx.height);
    uint32_t mask = g_rsx.clear_mask;
    float rr = ((g_rsx.clear_color >> 16) & 0xFF) / 255.f;
    float gg = ((g_rsx.clear_color >> 8) & 0xFF) / 255.f;
    float bb = (g_rsx.clear_color & 0xFF) / 255.f;
    float col[4] = { rr, gg, bb, 1.f };
    try {
        g_ctx11->OMSetRenderTargets(1, &g_rtv, g_dsv);
        if (g_dss) g_ctx11->OMSetDepthStencilState(g_dss, g_rsx.clear_stencil);
        g_ctx11->ClearRenderTargetView(g_rtv, col);
        if (g_dsv) {
            UINT df = 0;
            if (mask & 0x02) df |= D3D11_CLEAR_DEPTH;
            if (mask & 0x04) df |= D3D11_CLEAR_STENCIL;
            if (!df) df = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
            g_ctx11->ClearDepthStencilView(g_dsv, df, g_rsx.clear_depth, g_rsx.clear_stencil);
        }
        // Phase XV fixed-function: tinted overlay when draws occurred this frame
        if (g_rsx.draws_this_frame > 0) {
            float pr = ((g_rsx.prim_color >> 16) & 0xFF) / 255.f;
            float pg = ((g_rsx.prim_color >> 8) & 0xFF) / 255.f;
            float pb = (g_rsx.prim_color & 0xFF) / 255.f;
            float ov[4] = {
                col[0] * 0.55f + pr * 0.45f,
                col[1] * 0.55f + pg * 0.45f,
                col[2] * 0.55f + pb * 0.45f,
                1.f
            };
            g_ctx11->ClearRenderTargetView(g_rtv, ov);
            uint32_t pc = g_rsx.prim_color;
            uint32_t r8 = (pc >> 16) & 0xFF, g8 = (pc >> 8) & 0xFF, b8 = pc & 0xFF;
            r8 = (r8 + 1) & 0xFF; g8 = (g8 + 2) & 0xFF; b8 = (b8 + 3) & 0xFF;
            g_rsx.prim_color = (r8 << 16) | (g8 << 8) | b8;
        }
        g_rsx.draws_this_frame = 0;
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
        if (method == 0x01D8 || method == 0x1D8) {
            ++g_rsx.clears;
            if (arg > 0xFF) g_rsx.clear_color = arg;
            else g_rsx.clear_mask = arg;
        }
        else if (method == 0x01D4 || method == 0x1D4) {
            g_rsx.clear_mask = arg;
            if (arg & 0x01) ++g_rsx.clears;
            if (arg & 0x02) ++g_rsx.depth_clears;
            if (arg & 0x04) ++g_rsx.stencil_clears;
        }
        else if (method == 0x01DC || method == 0x1DC) {
            g_rsx.clear_depth = (arg == 0) ? 0.f : 1.f;
            ++g_rsx.depth_clears;
        }
        else if (method == 0x01E0 || method == 0x1E0) {
            g_rsx.clear_stencil = (uint8_t)(arg & 0xFF);
            ++g_rsx.stencil_clears;
        }
        else if (method == 0x0B00 || method == 0x0B04 || method == 0x0B08) {}
        else if (method == 0x0208 || method == 0x020C || method == 0x0A00 || method == 0x0A04) {}
        else if (method == 0x1800 || method == 0x1810 || method == 0x1818 ||
                 method == 0x1808 || method == 0x1820) {
            ++g_rsx.draws;
            ++g_rsx.draws_this_frame;
        }
        else if (method == 0x1D60 || method == 0x1D64 || method == 0x1D6C ||
                 method == 0x09B0 || method == 0x09B4) {
            ++g_rsx.shader_binds;
        }
        else if (method == 0x1A00 || method == 0x1A04 || method == 0x1A0C ||
                 method == 0x1840 || method == 0x1844) {
            ++g_rsx.tex_binds;
        }
        else if (method == 0x08C0 || method == 0x08C4 || method == 0x08C8 || method == 0x08CC) {
            if (method == 0x08C0) g_rsx.viewport_w = (arg >> 16) ? (arg >> 16) : g_rsx.viewport_w;
            if (method == 0x08C4) g_rsx.viewport_h = (arg >> 16) ? (arg >> 16) : g_rsx.viewport_h;
        }
        else if (method == 0x08D0 || method == 0x08D4 || method == 0x08D8 || method == 0x08DC) {
            if (method == 0x08D0) { g_rsx.scissor_x = arg & 0xFFFF; g_rsx.scissor_w = (arg >> 16) & 0xFFFF; }
            if (method == 0x08D4) { g_rsx.scissor_y = arg & 0xFFFF; g_rsx.scissor_h = (arg >> 16) & 0xFFFF; }
        }
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
        ps3rt_log("[rsx] flip #%llu draws=%d clears=%d depth=%d stencil=%d shaders=%d tex=%d methods=%d known=%d unknown=%d %ux%u\n",
            (unsigned long long)g_rsx.flip_count, g_rsx.draws, g_rsx.clears,
            g_rsx.depth_clears, g_rsx.stencil_clears,
            g_rsx.shader_binds, g_rsx.tex_binds,
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
