// Phase XV+XIX: RSX + depth/stencil + draw path + FIFO batching / coalesced Present
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <mutex>
#include <cstring>
#include <cstdio>
#include <vector>
#include <cstdint>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <wincodec.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#endif

static std::mutex g_rsx_mu;
static int g_backend = 1;

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
    int shader_binds = 0;
    int tex_binds = 0;
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t clear_color = 0x101828;
    uint32_t clear_mask = 0xF1;
    float clear_depth = 1.f;
    uint8_t clear_stencil = 0;
    uint32_t prim_color = 0x4060A0;
    int draws_this_frame = 0;
    uint32_t viewport_w = 1280;
    uint32_t viewport_h = 720;
    uint32_t scissor_x = 0, scissor_y = 0, scissor_w = 1280, scissor_h = 720;
};
static RsxState g_rsx;
static bool g_user_closed = false;

#if defined(_WIN32)
extern "C" const char* ps3rt_get_game_name(void);
static HWND g_hwnd = nullptr;
static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx11 = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static ID3D11DepthStencilView* g_dsv = nullptr;
static ID3D11Texture2D* g_depth_tex = nullptr;
static ID3D11DepthStencilState* g_dss = nullptr;

extern "C" PS3RT_API void ps3rt_force_exit(void);
extern "C" PS3RT_API int ps3rt_should_exit(void);

static LRESULT CALLBACK RsxWndProc(HWND hwnd, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE || m == WM_DESTROY) {
        g_user_closed = true;
        ps3rt_log("[rsx] window closed by user — exiting\n");
        ps3rt_force_exit();
        return 0;
    }
    return DefWindowProcA(hwnd, m, w, l);
}

static void rsx_release_depth() {
    if (g_dsv) { g_dsv->Release(); g_dsv = nullptr; }
    if (g_depth_tex) { g_depth_tex->Release(); g_depth_tex = nullptr; }
}

static void rsx_ensure_depth(uint32_t w, uint32_t h) {
    if (!g_dev || !g_ctx11) return;
    if (g_dsv && g_depth_tex) return;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (FAILED(g_dev->CreateTexture2D(&td, nullptr, &g_depth_tex))) return;
    D3D11_DEPTH_STENCIL_VIEW_DESC dvd{};
    dvd.Format = td.Format; dvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(g_dev->CreateDepthStencilView(g_depth_tex, &dvd, &g_dsv))) {
        rsx_release_depth(); return;
    }
    if (!g_dss) {
        D3D11_DEPTH_STENCIL_DESC dsd{};
        dsd.DepthEnable = TRUE; dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        dsd.StencilEnable = TRUE; dsd.StencilReadMask = 0xFF; dsd.StencilWriteMask = 0xFF;
        dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        dsd.BackFace = dsd.FrontFace;
        g_dev->CreateDepthStencilState(&dsd, &g_dss);
    }
}


#if defined(_WIN32)
// Load ../ICON0.PNG (PS3_GAME when EXE is in USRDIR) for window + taskbar only.
static HICON load_icon0_png_as_hicon() {
    char mod[MAX_PATH] = {};
    if (!GetModuleFileNameA(nullptr, mod, MAX_PATH)) return nullptr;
    char* slash = strrchr(mod, '\\');
    if (!slash) return nullptr;
    *slash = 0;
    // XX-O: USRDIR -> ../ICON0.PNG (PS3_GAME); forward slashes avoid C4129
    char candidates[8][MAX_PATH];
    std::snprintf(candidates[0], MAX_PATH, "%s/../ICON0.PNG", mod);
    std::snprintf(candidates[1], MAX_PATH, "%s/../PS3_GAME/ICON0.PNG", mod);
    std::snprintf(candidates[2], MAX_PATH, "%s/ICON0.PNG", mod);
    std::snprintf(candidates[3], MAX_PATH, "%s/PS3_GAME/ICON0.PNG", mod);
    std::snprintf(candidates[4], MAX_PATH, "%s/../../ICON0.PNG", mod);
    std::snprintf(candidates[5], MAX_PATH, "%s/../../PS3_GAME/ICON0.PNG", mod);
    std::snprintf(candidates[6], MAX_PATH, "%s/ICON0.png", mod);
    std::snprintf(candidates[7], MAX_PATH, "%s/../icon0.png", mod);
    const char* path = nullptr;
    for (int i = 0; i < 8; ++i) {
        DWORD a = GetFileAttributesA(candidates[i]);
        if (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY)) {
            path = candidates[i]; break;
        }
    }
    if (!path) return nullptr;
    HRESULT hr_init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) { if (SUCCEEDED(hr_init)) CoUninitialize(); return nullptr; }
    wchar_t wpath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);
    IWICBitmapDecoder* decoder = nullptr;
    hr = factory->CreateDecoderFromFilename(wpath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    HICON hicon = nullptr;
    if (SUCCEEDED(hr) && decoder) {
        IWICBitmapFrameDecode* frame = nullptr;
        if (SUCCEEDED(decoder->GetFrame(0, &frame)) && frame) {
            IWICFormatConverter* conv = nullptr;
            if (SUCCEEDED(factory->CreateFormatConverter(&conv)) && conv) {
                if (SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) {
                    UINT w = 0, h = 0; conv->GetSize(&w, &h);
                    if (w && h) {
                        UINT stride = w * 4, size = stride * h;
                        std::vector<BYTE> pixels(size);
                        if (SUCCEEDED(conv->CopyPixels(nullptr, stride, size, pixels.data()))) {
                            ICONINFO ii{}; ii.fIcon = TRUE;
                            BITMAPINFO bmi{}; bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                            bmi.bmiHeader.biWidth = (LONG)w; bmi.bmiHeader.biHeight = -(LONG)h;
                            bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biCompression = BI_RGB;
                            void* bits = nullptr;
                            HDC hdc = GetDC(nullptr);
                            HBITMAP color = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
                            if (color && bits) {
                                memcpy(bits, pixels.data(), size);
                                HBITMAP mask = CreateBitmap((int)w, (int)h, 1, 1, nullptr);
                                ii.hbmColor = color; ii.hbmMask = mask;
                                hicon = CreateIconIndirect(&ii);
                                if (mask) DeleteObject(mask);
                                DeleteObject(color);
                            }
                            ReleaseDC(nullptr, hdc);
                        }
                    }
                }
                conv->Release();
            }
            frame->Release();
        }
        decoder->Release();
    }
    factory->Release();
    if (SUCCEEDED(hr_init)) CoUninitialize();
    if (hicon) ps3rt_log("[rsx] XX-O window/taskbar icon from %s\n", path);
    return hicon;
}
static void apply_window_icons(HWND hwnd) {
    if (!hwnd) return;
    HICON big = load_icon0_png_as_hicon();
    if (big) {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)big);
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)big);
    }
}
#endif

static void rsx_host_init(uint32_t w, uint32_t h) {
    if (g_hwnd) return;
    g_rsx.width = w ? w : 1280; g_rsx.height = h ? h : 720;
    WNDCLASSA wc{}; wc.lpfnWndProc = RsxWndProc; wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "PPSX33_RSX"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);
    const char* gname = ps3rt_get_game_name();
    char title[256];
    if (gname && gname[0]) std::snprintf(title, sizeof(title), "%s — PPSX33", gname);
    else std::snprintf(title, sizeof(title), "PPSX33");
    g_hwnd = CreateWindowExA(0, "PPSX33_RSX", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        80, 80, (int)g_rsx.width + 16, (int)g_rsx.height + 39, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) return;
    apply_window_icons(g_hwnd);
    apply_window_icons(g_hwnd);
    ps3rt_log("[rsx] XX-O window title=\"%s\"\n", title);
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Width = g_rsx.width; sd.BufferDesc.Height = g_rsx.height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx11))) {
        ps3rt_log("[rsx] D3D11 create failed\n"); return;
    }
    ID3D11Texture2D* bb = nullptr;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (bb) { g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv); bb->Release(); }
    rsx_ensure_depth(g_rsx.width, g_rsx.height);
    ps3rt_log("[rsx] D3D11 host present ready %ux%u (depth+stencil)\n", g_rsx.width, g_rsx.height);
}

static void rsx_host_present_clear() {
    if (ps3rt_should_exit() || g_user_closed) { ps3rt_force_exit(); return; }
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { g_user_closed = true; ps3rt_force_exit(); return; }
        TranslateMessage(&msg); DispatchMessageA(&msg);
    }
    if (!g_hwnd) rsx_host_init(g_rsx.width, g_rsx.height);
    RECT rc; if (g_hwnd && GetClientRect(g_hwnd, &rc)) {
        uint32_t nw = (uint32_t)(rc.right - rc.left), nh = (uint32_t)(rc.bottom - rc.top);
        if (nw > 0 && nh > 0 && (nw != g_rsx.width || nh != g_rsx.height)) {
            g_rsx.width = nw; g_rsx.height = nh;
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            if (g_swap) g_swap->ResizeBuffers(0, nw, nh, DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* bb = nullptr;
            if (g_swap) g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
            if (bb && g_dev) { g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv); bb->Release(); }
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
        if (g_rsx.draws_this_frame > 0) {
            float pr = ((g_rsx.prim_color >> 16) & 0xFF) / 255.f;
            float pg = ((g_rsx.prim_color >> 8) & 0xFF) / 255.f;
            float pb = (g_rsx.prim_color & 0xFF) / 255.f;
            float ov[4] = { col[0] * 0.55f + pr * 0.45f, col[1] * 0.55f + pg * 0.45f, col[2] * 0.55f + pb * 0.45f, 1.f };
            g_ctx11->ClearRenderTargetView(g_rtv, ov);
            uint32_t pc = g_rsx.prim_color;
            uint32_t r8 = (pc >> 16) & 0xFF, g8 = (pc >> 8) & 0xFF, b8 = pc & 0xFF;
            r8 = (r8 + 1) & 0xFF; g8 = (g8 + 2) & 0xFF; b8 = (b8 + 3) & 0xFF;
            g_rsx.prim_color = (r8 << 16) | (g8 << 8) | b8;
        }
        // Phase XIX: coalesce Present — skip empty frames
        static uint32_t s_present_skip = 0;
        bool need_present = (g_rsx.draws_this_frame > 0) || ((s_present_skip++ & 7) == 0);
        g_rsx.draws_this_frame = 0;
        if (need_present && g_swap)
            g_swap->Present(0, 0);
        // XX-N: GDI status overlay (title / boot progress) until real draws dominate
        if (g_hwnd) {
            HDC hdc = GetDC(g_hwnd);
            if (hdc) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(240, 240, 240));
                char line[256];
                const char* gn = ps3rt_get_game_name();
                std::snprintf(line, sizeof(line),
                    "%s  |  flips=%llu  draws=%u  PPSX33 XX-N",
                    (gn && gn[0]) ? gn : "PPSX33",
                    (unsigned long long)g_rsx.flip_count,
                    g_rsx.draws_this_frame);
                RECT r{ 12, 12, (LONG)g_rsx.width - 12, 48 };
                DrawTextA(hdc, line, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE);
                // soft shadow line for readability on bright frames
                SetTextColor(hdc, RGB(20, 20, 20));
                RECT r2{ 13, 13, (LONG)g_rsx.width - 11, 49 };
                DrawTextA(hdc, line, -1, &r2, DT_LEFT | DT_TOP | DT_SINGLELINE);
                SetTextColor(hdc, RGB(240, 240, 240));
                DrawTextA(hdc, line, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE);
                ReleaseDC(g_hwnd, hdc);
            }
        }
    } catch (...) {
        ps3rt_log("[rsx] present exception — switching headless\n");
        g_user_closed = true;
    }
}
#else
static void rsx_host_init(uint32_t, uint32_t) {}
static void rsx_host_present_clear() {}
static HWND g_hwnd = nullptr;
#endif

static void rsx_process_fifo(uint8_t* mem) {
    if (!mem || !g_rsx.fifo_addr || !g_rsx.fifo_size) return;
    uint32_t put = g_rsx.put % g_rsx.fifo_size;
    uint32_t get = g_rsx.get % g_rsx.fifo_size;
    int steps = 0;
    while (get != put && steps < 16384) {
        uint64_t fa = g_rsx.fifo_addr + get;
        if (fa + 8 > 512ull * 1024 * 1024) break;
        uint32_t hdr = (mem[fa] << 24) | (mem[fa+1] << 16) | (mem[fa+2] << 8) | mem[fa+3];
        uint32_t arg = (mem[fa+4] << 24) | (mem[fa+5] << 16) | (mem[fa+6] << 8) | mem[fa+7];
        uint32_t method = hdr & 0x1FFC;
        ++g_rsx.methods_seen;
        ++g_rsx.known_methods;
        if (method == 0x01D8 || method == 0x1D8) {
            g_rsx.clear_color = arg & 0xFFFFFF;
            ++g_rsx.clears;
        }
        else if (method == 0x01D4 || method == 0x1D4) {
            g_rsx.clear_mask = arg;
            if (arg & 0x02) ++g_rsx.depth_clears;
            if (arg & 0x04) ++g_rsx.stencil_clears;
            ++g_rsx.clears;
        }
        else if (method == 0x01DC || method == 0x1DC) {
            g_rsx.clear_depth = (float)(arg & 0xFFFF) / 65535.f;
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
    return g_rsx.flip_count;
}
