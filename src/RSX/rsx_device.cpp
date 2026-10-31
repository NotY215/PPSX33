// WORK: RSX/libgcm host path — Vulkan 1.2 default, OpenGL secondary. DX11 removed.
#include "rsx_device.h"
#include "rsx_fifo.h"
#include "rsx_method_table.h"
#include "rsx_present_queue.h"
#include "rsx_draw_batch.h"
#include "rsx_surface_fmt.h"
#include "rsx_render_frame.h"
#include "ps3rt_be.h"

static void rsx_force_720p(uint32_t* w, uint32_t* h) {
    if (w) *w = 1280;
    if (h) *h = 720;
}

#include "rsx_framebuffer.h"
#include "rsx_memory.h"
#include "rsx_state.h"
#include "rsx_texture.h"
#include "rsx_shader_compiler.h"
#include "rsx_shader_decoder.h"
#include "Backends/Vulkan/rsx_vk_backend.h"
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <mutex>
#include <windows.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

#ifndef PS3RT_API
#  if defined(_WIN32) && defined(PS3RT_BUILD_DLL)
#    define PS3RT_API extern "C" __declspec(dllexport)
#  elif defined(_WIN32)
#    define PS3RT_API extern "C" __declspec(dllimport)
#  else
#    define PS3RT_API extern "C"
#  endif
#endif

extern "C" void ps3rt_log(const char* fmt, ...);
extern "C" const char* ps3rt_get_game_name(void);
extern "C" uint8_t* ps3rt_guest_mem_ptr(void);
extern "C" size_t ps3rt_guest_mem_size(void);
PS3RT_API int  ps3rt_should_exit(void);
PS3RT_API void ps3rt_force_exit(void);

static int g_backend = 0; /* 0=Vulkan1.2 1=OpenGL */

struct RsxState {
    uint32_t width = 1280, height = 720;
    uint64_t fifo_addr = 0;
    uint32_t fifo_size = 0;
    uint32_t put = 0, get = 0;
    uint32_t color_offset = 0;
    uint32_t color_pitch = 0;
    uint64_t flip_count = 0;
    uint32_t methods_seen = 0, unknown_methods = 0, draws_guest_frame = 0;
    uint32_t known_methods = 0, methods_this_frame = 0, draws_guest = 0, draws = 0;
    int draws_diagnostic = 0;
    uint32_t tex_offset = 0;
    uint32_t fifo_bytes_consumed = 0, packets_decoded = 0;
    bool flip_pending = false;
};
static RsxState g_rsx;
static std::mutex g_rsx_mu;
static HWND g_hwnd = nullptr;
static int g_gfx_api = 0; /* 0=Vulkan1.2 1=OpenGL */
static int g_draw_count = 0;
static uint32_t g_va_offset[8] = {};

static void rsx_surface_bind_color(uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h) {
    g_rsx.color_offset = offset;
    g_rsx.color_pitch = pitch;
    if (w) g_rsx.width = w;
    if (h) g_rsx.height = h;
    ppsx33::rsx::rsx_framebuffer_bind_color(offset, pitch, w, h, 0);
}


static LRESULT CALLBACK RsxWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) { DestroyWindow(h); return 0; }
    if (m == WM_DESTROY) { g_hwnd = nullptr; PostQuitMessage(0); return 0; }
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; BeginPaint(h, &ps); EndPaint(h, &ps);
        if (g_gfx_api == 0 && rsx_vk_ready())
            rsx_vk_present_clear(0.03f, 0.06f, 0.12f, 1.0f);
        else {
            HDC hdc = GetDC(h);
            if (hdc) {
                RECT rc; GetClientRect(h, &rc);
                HBRUSH br = CreateSolidBrush(RGB(8, 16, 32));
                FillRect(hdc, &rc, br); DeleteObject(br); ReleaseDC(h, hdc);
            }
        }
        return 0;
    }
    if (m == WM_ERASEBKGND) return 1;
    return DefWindowProcA(h, m, w, l);
}

#if defined(_WIN32)
// Resolve PS3_GAME path: EXE in USRDIR → parent is PS3_GAME
static int rsx_game_root(char* out, size_t out_sz) {
    if (!out || out_sz < 8) return 0;
    DWORD n = GetModuleFileNameA(nullptr, out, (DWORD)out_sz);
    if (!n || n >= out_sz) return 0;
    char* slash = strrchr(out, '\\');
    if (!slash) slash = strrchr(out, '/');
    if (!slash) return 0;
    *slash = 0; // drop EXE name → USRDIR
    slash = strrchr(out, '\\');
    if (!slash) slash = strrchr(out, '/');
    if (!slash) return 0;
    *slash = 0; // drop USRDIR → PS3_GAME
    return 1;
}

// PNG/JPG → HICON via Windows Imaging Component (windowscodecs)
static HICON rsx_load_png_as_hicon(const char* path, int cx, int cy) {
    if (!path || !path[0]) return nullptr;
    HRESULT hr_co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool need_uninit = SUCCEEDED(hr_co);
    HICON hicon = nullptr;
    IWICImagingFactory* factory = nullptr;
    hr_co = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, (void**)&factory);
    if (FAILED(hr_co) || !factory) {
        if (need_uninit) CoUninitialize();
        return nullptr;
    }
    wchar_t wpath[512];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, 512);
    IWICBitmapDecoder* dec = nullptr;
    hr_co = factory->CreateDecoderFromFilename(wpath, nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, &dec);
    if (SUCCEEDED(hr_co) && dec) {
        IWICBitmapFrameDecode* frame = nullptr;
        if (SUCCEEDED(dec->GetFrame(0, &frame)) && frame) {
            IWICFormatConverter* conv = nullptr;
            if (SUCCEEDED(factory->CreateFormatConverter(&conv)) && conv) {
                if (SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
                        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) {
                    UINT w = 0, h = 0;
                    conv->GetSize(&w, &h);
                    if (w && h) {
                        UINT stride = w * 4;
                        UINT sz = stride * h;
                        BYTE* bits = (BYTE*)malloc(sz);
                        if (bits && SUCCEEDED(conv->CopyPixels(nullptr, stride, sz, bits))) {
                            // Scale via StretchDIBits into DIB section if size mismatch
                            int dw = cx > 0 ? cx : (int)w;
                            int dh = cy > 0 ? cy : (int)h;
                            BITMAPV5HEADER bi{};
                            bi.bV5Size = sizeof(bi);
                            bi.bV5Width = dw;
                            bi.bV5Height = -dh;
                            bi.bV5Planes = 1;
                            bi.bV5BitCount = 32;
                            bi.bV5Compression = BI_BITFIELDS;
                            bi.bV5RedMask = 0x00FF0000;
                            bi.bV5GreenMask = 0x0000FF00;
                            bi.bV5BlueMask = 0x000000FF;
                            bi.bV5AlphaMask = 0xFF000000;
                            void* dib_bits = nullptr;
                            HDC hdc = GetDC(nullptr);
                            HBITMAP hbmp = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &dib_bits, nullptr, 0);
                            if (hbmp && dib_bits) {
                                if (dw == (int)w && dh == (int)h) {
                                    memcpy(dib_bits, bits, sz);
                                } else {
                                    // nearest-neighbor scale
                                    const uint32_t* src = (const uint32_t*)bits;
                                    uint32_t* dst = (uint32_t*)dib_bits;
                                    for (int y = 0; y < dh; ++y) {
                                        int sy = y * (int)h / dh;
                                        for (int x = 0; x < dw; ++x) {
                                            int sx = x * (int)w / dw;
                                            dst[y * dw + x] = src[sy * (int)w + sx];
                                        }
                                    }
                                }
                                ICONINFO ii{};
                                ii.fIcon = TRUE;
                                ii.hbmColor = hbmp;
                                ii.hbmMask = CreateBitmap(dw, dh, 1, 1, nullptr);
                                hicon = CreateIconIndirect(&ii);
                                if (ii.hbmMask) DeleteObject(ii.hbmMask);
                                DeleteObject(hbmp);
                            }
                            ReleaseDC(nullptr, hdc);
                        }
                        free(bits);
                    }
                }
                conv->Release();
            }
            frame->Release();
        }
        dec->Release();
    }
    factory->Release();
    if (need_uninit) CoUninitialize();
    return hicon;
}

static void apply_window_icons(HWND hwnd) {
    if (!hwnd) return;
    char roots[3][512];
    int nroots = 0;
    char path[512];
    DWORD n = GetModuleFileNameA(nullptr, path, sizeof(path));
    if (n && n < sizeof(path)) {
        char* slash = strrchr(path, '\\');
        if (!slash) slash = strrchr(path, '/');
        if (slash) {
            *slash = 0; // USRDIR
            if (nroots < 3) { strncpy(roots[nroots], path, 511); roots[nroots][511]=0; nroots++; }
            slash = strrchr(path, '\\');
            if (!slash) slash = strrchr(path, '/');
            if (slash) {
                *slash = 0; // PS3_GAME
                if (nroots < 3) { strncpy(roots[nroots], path, 511); roots[nroots][511]=0; nroots++; }
            }
        }
    }
    HICON small_i = nullptr, big_i = nullptr;
    char path_icon0[560], path_pic1[560];
    for (int i = 0; i < nroots && (!small_i || !big_i); ++i) {
        std::snprintf(path_icon0, sizeof(path_icon0), "%s\\ICON0.PNG", roots[i]);
        std::snprintf(path_pic1, sizeof(path_pic1), "%s\\PIC1.PNG", roots[i]);
        if (!small_i) small_i = rsx_load_png_as_hicon(path_icon0, 32, 32);
        if (!big_i) big_i = rsx_load_png_as_hicon(path_pic1, 48, 48);
        if (!big_i) big_i = rsx_load_png_as_hicon(path_icon0, 48, 48);
        if (small_i || big_i) {
            ps3rt_log("[rsx] icons root=%s ICON0=%s PIC1=%s\n", roots[i],
                small_i ? "ok" : "miss", big_i ? "ok" : "miss");
        }
    }
    if (!small_i) small_i = big_i;
    if (big_i) {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)big_i);
        SetClassLongPtrA(hwnd, GCLP_HICON, (LONG_PTR)big_i);
    }
    if (small_i) {
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small_i);
        SetClassLongPtrA(hwnd, GCLP_HICONSM, (LONG_PTR)small_i);
    }
}
#endif


static void rsx_probe_api_preference() {
    static int once = 0;
    if (once) return;
    once = 1;
    if (g_backend == 1) g_gfx_api = 1;
    else { g_gfx_api = 0; g_backend = 0; }
    if (g_gfx_api == 0)
        ps3rt_log("[rsx] XXII-L API preference: Vulkan 1.2 (OpenGL secondary; DX11 removed)\n");
    else
        ps3rt_log("[rsx] XXII-L API preference: OpenGL (Vulkan secondary)\n");
}

static void rsx_host_present_soft();

#if defined(_WIN32)
static HANDLE g_pump_thread = nullptr;
static volatile long g_pump_run = 0;
static DWORD WINAPI rsx_pump_thread_main(void*) {
    static int tick = 0;
    while (InterlockedCompareExchange(&g_pump_run, 1, 1) == 1) {
        if (g_hwnd) {
            MSG msg;
            while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    InterlockedExchange(&g_pump_run, 0);
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            if (g_gfx_api == 0 && rsx_vk_ready()) {
                rsx_present_req req{};
                if (rsx_present_queue_pop(&req) && req.flags == 2 && req.color_offset) {
                    uint32_t w = 1280, h = 720;
                    uint32_t pitch = req.pitch ? req.pitch : (w * 4);
                    if (pitch < w * 4) pitch = w * 4;
                    uint8_t* mem = ps3rt_guest_mem_ptr();
                    size_t msz = ps3rt_guest_mem_size();
                    size_t need = (size_t)req.color_offset + (size_t)pitch * (size_t)h;
                    if (mem && req.color_offset < msz && need <= msz && need > 0)
                        rsx_vk_present_guest_rgba(mem + req.color_offset, w, h, pitch);
                    else
                        rsx_vk_present_clear(0.03f, 0.06f, 0.12f, 1.0f);
                } else if (((++tick) & 3) == 0) {
                    // clear at ~15Hz max — avoids VK cmd buffer races with flip path
                    rsx_vk_present_clear(0.03f, 0.06f, 0.12f, 1.0f);
                }
            } else if (g_hwnd && ((tick & 3) == 0))
                rsx_host_present_soft();
        }
        Sleep(16);
    }
    return 0;
}
static void rsx_start_pump_thread() {
    if (g_pump_thread) return;
    InterlockedExchange(&g_pump_run, 1);
    g_pump_thread = CreateThread(nullptr, 0, rsx_pump_thread_main, nullptr, 0, nullptr);
    if (g_pump_thread)
        ps3rt_log("[rsx] HWND pump thread started\n");
}
#else
static void rsx_start_pump_thread() {}
#endif

static void rsx_host_init(uint32_t w, uint32_t h) {
    if (g_hwnd) return;
    rsx_probe_api_preference();
    g_rsx.width = 1280;
    g_rsx.height = 720;
    (void)w; (void)h;
    WNDCLASSA wc{};
    wc.lpfnWndProc = RsxWndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "PPSX33_RSX";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);
    const char* gname = ps3rt_get_game_name();
    char title[256];
    if (gname && gname[0]) std::snprintf(title, sizeof(title), "PPSX33 presents %s", gname);
    else std::snprintf(title, sizeof(title), "PPSX33 presents PS3 Title");
    g_hwnd = CreateWindowExA(0, "PPSX33_RSX", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        80, 80, (int)g_rsx.width + 16, (int)g_rsx.height + 39, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) return;
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    ps3rt_log("[rsx] XX-O window title=\"%s\"\n", title);
    rsx_start_pump_thread();
    if (g_gfx_api == 0) {
        if (rsx_vk_init((void*)g_hwnd, g_rsx.width, g_rsx.height)) {
            ps3rt_log("[rsx] host present ready %ux%u (Vulkan 1.2)\n",
                (unsigned)g_rsx.width, (unsigned)g_rsx.height);
            rsx_vk_poll_window();
        } else {
            ps3rt_log("[rsx] Vulkan init failed - soft GDI present fallback\n");
            rsx_host_present_soft();
        }
    } else {
        ps3rt_log("[rsx] host present ready %ux%u (OpenGL path; soft clear interim)\n",
            (unsigned)g_rsx.width, (unsigned)g_rsx.height);
        rsx_host_present_soft();
    }
#if defined(_WIN32)
    apply_window_icons(g_hwnd);
#endif
}

static void rsx_host_present_soft() {
    if (!g_hwnd) return;
    HDC hdc = GetDC(g_hwnd);
    if (!hdc) return;
    RECT rc{}; GetClientRect(g_hwnd, &rc);
    HBRUSH br = CreateSolidBrush(RGB(8, 16, 32));
    FillRect(hdc, &rc, br);
    DeleteObject(br);
    RECT bar{ 0, 0, rc.right, 6 };
    HBRUSH br2 = CreateSolidBrush(RGB(56, 189, 248));
    FillRect(hdc, &bar, br2);
    DeleteObject(br2);
    ReleaseDC(g_hwnd, hdc);
}

static void rsx_host_present_clear() {
    if (!g_hwnd) return;
    if (g_gfx_api == 0 && rsx_vk_ready()) {
        // Dark blue clear via Vulkan swapchain
        rsx_vk_present_clear(0.03f, 0.06f, 0.12f, 1.0f);
        rsx_vk_poll_window();
        return;
    }
    rsx_host_present_soft();
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { g_hwnd = nullptr; break; }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static bool rsx_state_apply_method(uint32_t method, uint32_t arg) {
    rsx_method_kind kind = rsx_classify_method(method);
    if (kind == RSX_M_SURFACE_COLOR_OFFSET) {
        rsx_surface_bind_color(arg, g_rsx.color_pitch, g_rsx.width, g_rsx.height);
        rsx_draw_batch_set_color(arg, g_rsx.color_pitch, 1280, 720);
        ppsx33::rsx::rsx_framebuffer_bind_color(arg, g_rsx.color_pitch, g_rsx.width, g_rsx.height, 0);
        return true;
    }
    if (kind == RSX_M_TEXTURE_OFFSET) {
        g_rsx.tex_offset = arg;
        ppsx33::rsx::rsx_texture_bind(0, arg, 0, g_rsx.width, g_rsx.height, 0);
        return true;
    }
    if (kind == RSX_M_DRAW) {
        ++g_rsx.draws;
        rsx_draw_batch_note_draw();
        return true;
    }
    if (kind == RSX_M_CLEAR) {
        rsx_draw_batch_note_clear(1);
        return true;
    }
    if (kind == RSX_M_SURFACE_COLOR_OFFSET) {
        /* already handled above in some paths */
    }
    // NV4097-class methods (method is byte offset style as seen in FIFO decode)
    switch (method) {
    case 0x182c: case 0x1830: // color surface offset variants
        rsx_surface_bind_color(arg, g_rsx.color_pitch, g_rsx.width, g_rsx.height);
        ppsx33::rsx::rsx_framebuffer_bind_color(arg, g_rsx.color_pitch, g_rsx.width, g_rsx.height, 0);
        return true;
    case 0x1d88: case 0x1a00: // texture offset unit0
        g_rsx.tex_offset = arg;
        ppsx33::rsx::rsx_texture_bind(0, arg, 0, g_rsx.width, g_rsx.height, 0);
        return true;
    case 0x8c0: case 0x8c4: // viewport-ish
        if (arg) ppsx33::rsx::rsx_state_set_viewport(arg & 0xffffu, (arg >> 16) & 0xffffu);
        return true;
    case 0x8c8: case 0x8cc:
        ppsx33::rsx::rsx_state_set_scissor(arg & 0xffffu, (arg >> 16) & 0xffffu, g_rsx.width, g_rsx.height);
        return true;
    default:
        if (method <= 0x2000u || (method >= 0x1800u && method <= 0x1C00u)) return true;
        return false;
    }
}
static bool rsx_state_apply_method_p2(uint32_t method, uint32_t arg) {
    (void)method; (void)arg; return false;
}
static void rsx_draw_host(uint8_t* mem) {
    (void)mem;
    ++g_rsx.draws_guest_frame;
    ++g_rsx.draws;
    ++g_rsx.draws_diagnostic;
    ppsx33::rsx::rsx_state_set_draw(4 /* triangles */, g_draw_count ? (uint32_t)g_draw_count : 3u);
    // Present guest color if bound; else clear
    int presented = 0;
    if (g_gfx_api == 0 && rsx_vk_ready() && g_rsx.color_offset) {
        uint8_t* gmem = ps3rt_guest_mem_ptr();
        size_t msz = ps3rt_guest_mem_size();
        auto fb = ppsx33::rsx::rsx_framebuffer_color();
        uint32_t off = fb.offset ? fb.offset : g_rsx.color_offset;
        uint32_t gw = 1280;
        uint32_t gh = 720;
        uint32_t pitch = fb.pitch ? fb.pitch : gw * 4u;
        if (gmem && (size_t)off + (size_t)pitch * gh <= msz)
            presented = rsx_vk_present_guest_rgba(gmem + off, gw, gh, pitch);
    }
    if (!presented) rsx_host_present_clear();
}



static void rsx_fifo_decode_word(uint8_t* mem, uint32_t method, uint32_t arg) {
    (void)mem;
    if (rsx_state_apply_method(method, arg) || rsx_state_apply_method_p2(method, arg)) {
        ++g_rsx.known_methods;
        ++g_rsx.methods_seen;
        ++g_rsx.methods_this_frame;
        // Issue host draw when guest issues draw-class methods
        if (method == 0x1814 || method == 0x1824) {
            int before = g_rsx.draws_guest_frame;
            rsx_draw_host(mem);
            // Cmdstream draw must count as guest even if geometry falls back to solid
            if (g_rsx.draws_guest_frame == before) {
                ++g_rsx.draws_guest;
                ++g_rsx.draws_guest_frame;
                ++g_rsx.draws;
            }
            static int dg=0;
            if (dg < 24) {
                ps3rt_log("[rsx] XV-O draw method=0x%x arg=0x%x count=%u tex=0x%x color=0x%x guest_frame=%d\n",
                    method, arg, g_draw_count, g_rsx.tex_offset, (unsigned)g_rsx.color_offset,
                    g_rsx.draws_guest_frame);
                ++dg;
            }
        } else if (method == 0x1808 && arg == 0 && g_draw_count >= 3 && g_va_offset[0]) {
            rsx_draw_host(mem);
        }
    } else {
        ++g_rsx.unknown_methods;
        ++g_rsx.methods_seen;
        ++g_rsx.methods_this_frame;
        // XXVI: sample unknown methods so FIFO probe diagnosis is actionable
        static int s_unk = 0;
        if (s_unk < 32) {
            ps3rt_log("[rsx] XXVI unknown method=0x%x arg=0x%x (seen=%d unk=%d)\n",
                method, arg, g_rsx.methods_seen, g_rsx.unknown_methods);
            ++s_unk;
        }
    }
}

static void rsx_process_fifo(uint8_t* mem) {
    if (!mem || !g_rsx.fifo_addr || !g_rsx.fifo_size) return;
    uint32_t base = (uint32_t)(g_rsx.fifo_addr & 0xffffffffu);
    if (base == 0 || base >= 0x10000000u) return;
    uint32_t fsz0 = g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u;
    uint32_t put = g_rsx.put % fsz0;
    uint32_t get = g_rsx.get % fsz0;
    g_rsx.put = put;
    g_rsx.get = get;
    if (put == get) return;
    {
        // AIFIAR: do not drop the stream when GET is stale-ahead of PUT after a ring restart.
        // Log evidence: get=0xd0 put=0x68 dist=0xfff98 — false "full ring" wipe skipped real cmds.
        if (put < get && put < 0x10000u && get < 0x10000u) {
            // Log: get=0x548 put=0x4e0 — GET slightly ahead is STALE, not a full-ring wrap.
            uint32_t ahead = get - put;
            static int s_rw = 0;
            if (ahead < 0x10000u) {
                if (s_rw < 16) {
                    ps3rt_log("[rsx] FIFO stale GET snap get=0x%x -> put=0x%x (ahead=0x%x)\n",
                        get, put, ahead);
                    ++s_rw;
                }
                get = put;
                g_rsx.get = put;
            } else {
                static uint32_t s_last_put = 0;
                uint32_t start = (put > s_last_put) ? s_last_put : 0;
                if (s_rw < 16) {
                    ps3rt_log("[rsx] FIFO PUT restart get=0x%x -> 0x%x (put=0x%x)\n", get, start, put);
                    ++s_rw;
                }
                get = start;
                g_rsx.get = start;
                s_last_put = put;
            }
        } else {
            uint32_t dist = (put >= get) ? (put - get) : (fsz0 - get + put);
            if (dist > (fsz0 * 3u / 4u)) {
                static int s_rs = 0;
                if (s_rs < 8) {
                    ps3rt_log("[rsx] FIFO large gap get=0x%x put=0x%x dist=0x%x (process capped)\n",
                        get, put, dist);
                    ++s_rs;
                }
                // Do NOT set get=put; process with kMax cap below
            }
        }
    }
    uint32_t steps = 0;
    const uint32_t kMax = 0x800;
    while (get != put && steps < kMax) {
        uint32_t off = base + (get % (g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u));
        if (off + 8u >= 0x10000000u) break;
        uint32_t hdr_be = ((uint32_t)mem[off] << 24) | ((uint32_t)mem[off+1] << 16) |
                          ((uint32_t)mem[off+2] << 8) | (uint32_t)mem[off+3];
        uint32_t hdr_le = (uint32_t)mem[off] | ((uint32_t)mem[off+1] << 8) |
                          ((uint32_t)mem[off+2] << 16) | ((uint32_t)mem[off+3] << 24);
        // Guest cmdstream is BE; fall back to LE only if BE looks inert
        uint32_t hdr = hdr_be;
        if ((hdr_be & 0x1FFCu) == 0 && (hdr_le & 0x1FFCu) != 0) hdr = hdr_le;
        // Do NOT bswap again — hdr_be/hdr_le already absolute word values
        uint32_t method = hdr & 0x1FFCu;
        uint32_t count = ((hdr >> 18) & 0x7FFu);
        if (count == 0) count = 1;
        uint32_t subch = (hdr >> 13) & 7;
        (void)subch;
        // XV-B depth: JUMP / CALL / RETURN
        static uint32_t s_fifo_ret = 0;
        if ((hdr & 0xE0000003u) == 0x20000000u) {
            uint32_t fszj = g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u;
            uint32_t target = (hdr & 0x1FFFFFFCu) % fszj;
            get = target;
            g_rsx.get = get;
            ++g_rsx.packets_decoded;
            ++steps;
            static int jn = 0;
            if (jn < 12) { ps3rt_log("[rsx] XV-B JUMP -> 0x%x\n", target); ++jn; }
            continue;
        }
        if ((hdr & 0xE0000003u) == 0x00000002u) { // CALL
            uint32_t fszc = g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u;
            s_fifo_ret = (get + 4) % fszc;
            uint32_t target = (hdr & 0x1FFFFFFCu) % fszc;
            get = target;
            g_rsx.get = get;
            ++g_rsx.packets_decoded;
            ++steps;
            static int cn = 0;
            if (cn < 12) { ps3rt_log("[rsx] XV-B CALL -> 0x%x\n", target); ++cn; }
            continue;
        }
        if ((hdr & 0xE0000003u) == 0x00020000u || hdr == 0x20000u) { // RETURN
            if (s_fifo_ret) { get = s_fifo_ret; s_fifo_ret = 0; }
            else get = (get + 4) % (g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u);
            g_rsx.get = get;
            ++g_rsx.packets_decoded;
            ++steps;
            continue;
        }
        get = (get + 4) % (g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u);
        for (uint32_t i = 0; i < count && get != put && steps < kMax; ++i) {
            uint32_t aoff = base + (get % (g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u));
            if (aoff + 4u >= 0x10000000u) break;
            uint32_t arg = (uint32_t)mem[aoff] | ((uint32_t)mem[aoff+1] << 8) | ((uint32_t)mem[aoff+2] << 16) | ((uint32_t)mem[aoff+3] << 24);
            arg = ((arg & 0xff) << 24) | ((arg & 0xff00) << 8) | ((arg >> 8) & 0xff00) | ((arg >> 24) & 0xff);
            get = (get + 4) % (g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u);
            rsx_fifo_decode_word(mem, method + (i * 4), arg);
            ++g_rsx.methods_this_frame;
            ++g_rsx.methods_seen;
            g_rsx.fifo_bytes_consumed += 4;
            ++steps;
        }
        ++g_rsx.packets_decoded;
        g_rsx.get = get;
    }

    // Never leave GET ahead of PUT on a non-wrapped low ring (inject/control race).
    if (g_rsx.put < 0x10000u && g_rsx.get < 0x10000u && g_rsx.get > g_rsx.put) {
        g_rsx.get = g_rsx.put;
    }
}

PS3RT_API void ps3rt_set_graphics_backend(int backend) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    g_backend = backend;
}

PS3RT_API int ps3rt_get_graphics_backend(void) { return g_backend; }

PS3RT_API void ps3rt_rsx_init(uint64_t fifo_addr, uint32_t fifo_size) {
    {
        std::lock_guard<std::mutex> lk(g_rsx_mu);
        g_rsx.fifo_addr = fifo_addr;
        g_rsx.fifo_size = fifo_size ? fifo_size : 0x100000;
        g_rsx.put = g_rsx.get = 0;
        ppsx33::rsx::rsx_fifo_init();
        rsx_present_queue_init();
        rsx_draw_batch_reset();
        ppsx33::rsx::rsx_framebuffer_init();
        ppsx33::rsx::rsx_memory_init();
        ppsx33::rsx::rsx_state_init();
        ppsx33::rsx::rsx_texture_init();
        ppsx33::rsx::rsx_shader_compiler_init();
        ppsx33::rsx::rsx_shader_decoder_init();
    }
    if (!g_hwnd) rsx_host_init(1280, 720);
    ps3rt_log("[rsx] init fifo=0x%llx size=0x%x backend=%d (guest-PUT probe ready)\n",
        (unsigned long long)fifo_addr, (unsigned)fifo_size, g_backend);
}


PS3RT_API void ps3rt_rsx_set_color_surface(uint32_t offset, uint32_t w, uint32_t h) {
    rsx_surface_bind_color(offset, w ? ((w*4+63)&~63u) : 0, w, h);
}

PS3RT_API void ps3rt_rsx_set_put(uint32_t put) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    uint32_t fsz = g_rsx.fifo_size ? g_rsx.fifo_size : 0x100000u;
    g_rsx.put = put % fsz;
}

PS3RT_API uint32_t ps3rt_rsx_get_get(void) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    return g_rsx.get;
}

PS3RT_API uint32_t ps3rt_rsx_get_put(void) {
    std::lock_guard<std::mutex> lk(g_rsx_mu);
    return g_rsx.put;
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
    int frame_methods = g_rsx.methods_this_frame;
    int frame_packets = g_rsx.packets_decoded;
    // Prefer guest color surface upload when bound and memory is live
    int presented = 0;
    if (g_gfx_api == 0 && rsx_vk_ready() && g_rsx.color_offset) {
        uint8_t* mem = ps3rt_guest_mem_ptr();
        size_t msz = ps3rt_guest_mem_size();
        uint32_t off = g_rsx.color_offset;
        uint32_t gw = 1280;
        uint32_t gh = 720;
        auto fb = ppsx33::rsx::rsx_framebuffer_color();
        if (fb.offset) off = fb.offset;
        uint32_t pitch = fb.pitch ? fb.pitch : (gw * 4u);
        if (pitch < gw * 4u) pitch = gw * 4u;
        size_t need = (size_t)off + (size_t)pitch * gh;
        if (mem && need <= msz) {
            if (rsx_render_frame_on_flip(off, pitch))
                presented = 1;
            else
                presented = rsx_vk_present_guest_rgba(mem + off, 1280, 720, pitch);
            if (presented) rsx_vk_poll_window();
        }
    }
    if (!presented)
        rsx_host_present_clear();
    if (g_rsx.flip_count <= 32 || (g_rsx.flip_count & 0xFF) == 1) {
        ps3rt_log("[rsx] flip #%llu guest=%d diag=%d draws=%d methods=%d packets=%d known=%d unk=%d put=0x%x get=0x%x %ux%u\n",
            (unsigned long long)g_rsx.flip_count, g_rsx.draws_guest, g_rsx.draws_diagnostic,
            g_rsx.draws, frame_methods, frame_packets,
            g_rsx.known_methods, g_rsx.unknown_methods,
            g_rsx.put, g_rsx.get,
            g_rsx.width, g_rsx.height);
    }
    g_rsx.methods_this_frame = 0;
    g_rsx.packets_decoded = 0;
}

PS3RT_API void ps3rt_rsx_tick(void) {
    if (g_hwnd) {
        if (g_gfx_api == 0 && rsx_vk_ready()) {
            static uint64_t s_tick = 0;
            if ((++s_tick & 0x3F) == 0)
                rsx_vk_present_clear(0.03f, 0.06f, 0.12f, 1.0f);
            rsx_vk_poll_window();
        } else {
            rsx_host_present_soft();
        }
    }
}

PS3RT_API uint64_t ps3rt_rsx_flip_count(void) {
    return g_rsx.flip_count;
}
