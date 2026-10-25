// NV40 vertex/fragment pipeline + guest vertex fetch draws
// XXV-K: guest color buffer paint for first real presentable frames
// VISIBLE FRAME FIX: no post-draw clear; no black guest wipe; always solid triangle
// XXV-H: C3861 forward decls for RSX helpers; SPU/MFC depth in companion files
// XXV-D FULL DEPTH (on XXV-E): part1 FIFO/surface/host draw + part2 vertex/depth/blend/viewport
// XXV-D: surface_bind + present_guest (RSX pipeline start)
// XXV-FRAME: visible present path (guest geometry still Phase XXV-D+)
// XXIV-E COLOR_OFFSET present + visible diagnostic
// XXIV-A/B/C companion in rsx: FIFO bound + HLSL prove
// XXIII-X/Y/Z: diagnostic frame + HLSL path + pitch align
// XXII-A: RSX present/color-buffer fidelity · // A+I+L+T+V+Y: + fullscreen blit of uploaded guest texture to backbuffer
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
#include <d3dcompiler.h>
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
    int methods_this_frame = 0; // XXIV-B
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
    // A: surface
    int color_blits = 0;
    uint64_t color_offset = 0;
    uint32_t color_pitch = 0;
    uint32_t surf_w = 1280, surf_h = 720;
    bool color_valid = false;
    // I: draw / shader
    uint32_t shader_prog = 0;
    uint32_t shader_const_count = 0;
    uint32_t vertex_array_mask = 0;
    uint32_t begin_mode = 0;
    int vertex_binds = 0;
    int fragment_binds = 0;
    // L: texture unit 0
    uint32_t tex_offset = 0;
    uint32_t tex_format = 0;
    uint32_t tex_width = 0;
    uint32_t tex_height = 0;
    uint32_t tex_pitch = 0;
    int tex_units_bound = 0;
    // XXIII-O: program bind surface
    uint32_t vp_offset = 0;   // vertex program
    uint32_t fp_offset = 0;   // fragment program
    uint32_t fp_control = 0;
    // XXIII-P: texture format surface
    uint32_t tex_mipmap = 0;
    uint32_t tex_depth = 1;
    uint32_t tex_swizzle = 0;
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
// V: host copy of guest texture (point-sampled blit source)
static ID3D11Texture2D* g_guest_tex = nullptr;
static ID3D11ShaderResourceView* g_guest_srv = nullptr;
static uint32_t g_guest_tex_w = 0, g_guest_tex_h = 0;
static uint64_t g_guest_tex_src = 0;

// XXIII-T: host HLSL blit pipeline (compiled at runtime via D3DCompile)
static ID3D11VertexShader* g_vs_blit = nullptr;
static ID3D11PixelShader* g_ps_blit = nullptr;
static ID3D11InputLayout* g_layout_blit = nullptr;
static ID3D11Buffer* g_vb_quad = nullptr;
static ID3D11SamplerState* g_samp = nullptr;
static ID3D11ShaderResourceView* g_srv_tex = nullptr;
static int g_hlsl_ready = 0;

static const char* kBlitHLSL = R"(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);
VSOut VSMain(VSIn i) {
  VSOut o;
  o.pos = float4(i.pos, 0, 1);
  o.uv = i.uv;
  return o;
}
float4 PSMain(VSOut i) : SV_Target {
  // XXIII-Y: sample guest texture; solid fallback tint if unbound
  float4 c = tex0.Sample(samp0, i.uv);
  return c;
}
)";

static bool rsx_ensure_hlsl() {
    if (g_hlsl_ready) return g_vs_blit && g_ps_blit;
    g_hlsl_ready = 1;
    if (!g_dev) return false;
    HMODULE d3dcomp = LoadLibraryA("d3dcompiler_47.dll");
    if (!d3dcomp) d3dcomp = LoadLibraryA("d3dcompiler_43.dll");
    if (!d3dcomp) {
        static int s = 0;
        if (s < 1) { ps3rt_log("[rsx] XXIV-C D3DCompile DLL missing — CPU blit only\n"); ++s; }
        return false;
    }
    typedef HRESULT (WINAPI *PFN_D3DCompile)(LPCVOID, SIZE_T, LPCSTR, const void*, const void*,
        LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
    auto pCompile = (PFN_D3DCompile)GetProcAddress(d3dcomp, "D3DCompile");
    if (!pCompile) return false;
    ID3DBlob *vsb = nullptr, *psb = nullptr, *err = nullptr;
    HRESULT hr = pCompile(kBlitHLSL, strlen(kBlitHLSL), "blit", nullptr, nullptr,
        "VSMain", "vs_4_0", 0, 0, &vsb, &err);
    if (FAILED(hr) || !vsb) {
        ps3rt_log("[rsx] XXIV-C VS compile fail\n");
        if (err) err->Release();
        return false;
    }
    hr = pCompile(kBlitHLSL, strlen(kBlitHLSL), "blit", nullptr, nullptr,
        "PSMain", "ps_4_0", 0, 0, &psb, &err);
    if (FAILED(hr) || !psb) {
        ps3rt_log("[rsx] XXIV-C PS compile fail\n");
        if (err) err->Release();
        vsb->Release();
        return false;
    }
    g_dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g_vs_blit);
    g_dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_ps_blit);
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    g_dev->CreateInputLayout(ied, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g_layout_blit);
    vsb->Release(); psb->Release();
    float quad[] = {
        -1,-1, 0,1,  1,-1, 1,1,  -1,1, 0,0,
        -1,1, 0,0,  1,-1, 1,1,  1,1, 1,0,
    };
    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = sizeof(quad); bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sd{ quad, 0, 0 };
    g_dev->CreateBuffer(&bd, &sd, &g_vb_quad);
    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    g_dev->CreateSamplerState(&samp, &g_samp);
    ps3rt_log("[rsx] XXIV-C HLSL blit VS/PS ready\n");
    return g_vs_blit && g_ps_blit;
}


// XXV-E/D: solid-color fullscreen/triangle path when no guest tex (still a real GPU draw)
static ID3D11VertexShader* g_vs_solid = nullptr;
static ID3D11PixelShader* g_ps_solid = nullptr;
static ID3D11InputLayout* g_layout_solid = nullptr;
static ID3D11Buffer* g_vb_tri = nullptr;

// Forward decls — definitions appear later (C3861 fix)
static void rsx_apply_viewport_scissor();
static void rsx_present_guest_or_diag(uint8_t* mem);
static void rsx_paint_guest_color_buffer(uint8_t* mem);
static bool rsx_ensure_nv40_pipeline();
static void rsx_draw_nv40_geometry(uint8_t* mem);
static bool rsx_state_apply_method_p2(uint32_t method, uint32_t arg);
static void rsx_draw_host(uint8_t* mem);
static void rsx_sync_guest_host(uint8_t* mem);

static int g_solid_ready = 0;

static const char* kSolidHLSL = R"(
struct VSIn { float2 pos : POSITION; float4 col : COLOR0; };
struct VSOut { float4 pos : SV_POSITION; float4 col : COLOR0; };
VSOut VSMain(VSIn i) { VSOut o; o.pos = float4(i.pos, 0, 1); o.col = i.col; return o; }
float4 PSMain(VSOut i) : SV_Target { return i.col; }
)";

static bool rsx_ensure_solid() {
    if (g_solid_ready) return g_vs_solid && g_ps_solid;
    g_solid_ready = 1;
    if (!g_dev) return false;
    if (!rsx_ensure_hlsl()) { /* need d3dcompiler load path */ }
    HMODULE d3dcomp = LoadLibraryA("d3dcompiler_47.dll");
    if (!d3dcomp) d3dcomp = LoadLibraryA("d3dcompiler_43.dll");
    if (!d3dcomp) return false;
    typedef HRESULT (WINAPI *PFN_D3DCompile)(LPCVOID, SIZE_T, LPCSTR, const void*, const void*,
        LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
    auto pCompile = (PFN_D3DCompile)GetProcAddress(d3dcomp, "D3DCompile");
    if (!pCompile) return false;
    ID3DBlob *vsb = nullptr, *psb = nullptr, *err = nullptr;
    if (FAILED(pCompile(kSolidHLSL, strlen(kSolidHLSL), "solid", nullptr, nullptr, "VSMain", "vs_4_0", 0, 0, &vsb, &err)) || !vsb) {
        if (err) err->Release();
        return false;
    }
    if (FAILED(pCompile(kSolidHLSL, strlen(kSolidHLSL), "solid", nullptr, nullptr, "PSMain", "ps_4_0", 0, 0, &psb, &err)) || !psb) {
        if (err) err->Release();
        vsb->Release();
        return false;
    }
    g_dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g_vs_solid);
    g_dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_ps_solid);
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    g_dev->CreateInputLayout(ied, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g_layout_solid);
    vsb->Release(); psb->Release();
    // Fullscreen-ish triangle with vertex colors (animated pulse via CPU rewrite each present)
    float tri[] = {
        -1.f,-1.f,  1,0,0,1,
         1.f,-1.f,  0,1,0,1,
         0.f, 1.f,  0,0,1,1,
    };
    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = sizeof(tri); bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA sd{ tri, 0, 0 };
    g_dev->CreateBuffer(&bd, &sd, &g_vb_tri);
    ps3rt_log("[rsx] XXV-E solid triangle VS/PS ready\n");
    return g_vs_solid && g_ps_solid;
}

static void rsx_draw_solid_triangle() {
    rsx_apply_viewport_scissor();
    if (!rsx_ensure_solid() || !g_ctx11 || !g_rtv || !g_vb_tri) return;
    // pulse colors
    D3D11_MAPPED_SUBRESOURCE map{};
    if (SUCCEEDED(g_ctx11->Map(g_vb_tri, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) {
        float t = (float)(g_rsx.flip_count % 256) / 255.f;
        float* v = (float*)map.pData;
        // 3 verts * (2 pos + 4 col)
        float cols[3][4] = {
            { 1.f, t, 0.2f, 1.f },
            { t, 1.f, 0.2f, 1.f },
            { 0.2f, t, 1.f, 1.f },
        };
        float pos[3][2] = { {-1.f,-1.f},{1.f,-1.f},{0.f,1.f} };
        for (int i = 0; i < 3; ++i) {
            v[i*6+0] = pos[i][0]; v[i*6+1] = pos[i][1];
            v[i*6+2] = cols[i][0]; v[i*6+3] = cols[i][1];
            v[i*6+4] = cols[i][2]; v[i*6+5] = cols[i][3];
        }
        g_ctx11->Unmap(g_vb_tri, 0);
    }
    UINT stride = 24, offset = 0;
    g_ctx11->IASetInputLayout(g_layout_solid);
    g_ctx11->IASetVertexBuffers(0, 1, &g_vb_tri, &stride, &offset);
    g_ctx11->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_ctx11->VSSetShader(g_vs_solid, nullptr, 0);
    g_ctx11->PSSetShader(g_ps_solid, nullptr, 0);
    g_ctx11->OMSetRenderTargets(1, &g_rtv, nullptr);
    D3D11_VIEWPORT vp{ 0, 0, (float)(g_rsx.width ? g_rsx.width : 1280), (float)(g_rsx.height ? g_rsx.height : 720), 0, 1 };
    g_ctx11->RSSetViewports(1, &vp);
    g_ctx11->Draw(3, 0);
    static int sd = 0;
    if (sd < 16) { ps3rt_log("[rsx] XXV-E solid triangle draw flip=%llu\n", (unsigned long long)g_rsx.flip_count); ++sd; }
}


static void rsx_draw_textured_quad() {
    if (!rsx_ensure_hlsl() || !g_ctx11 || !g_guest_tex || !g_rtv) return;
    if (g_srv_tex) { g_srv_tex->Release(); g_srv_tex = nullptr; }
    g_dev->CreateShaderResourceView(g_guest_tex, nullptr, &g_srv_tex);
    if (!g_srv_tex) return;
    UINT stride = 16, offset = 0;
    g_ctx11->IASetInputLayout(g_layout_blit);
    g_ctx11->IASetVertexBuffers(0, 1, &g_vb_quad, &stride, &offset);
    g_ctx11->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_ctx11->VSSetShader(g_vs_blit, nullptr, 0);
    g_ctx11->PSSetShader(g_ps_blit, nullptr, 0);
    g_ctx11->PSSetShaderResources(0, 1, &g_srv_tex);
    g_ctx11->PSSetSamplers(0, 1, &g_samp);
    g_ctx11->OMSetRenderTargets(1, &g_rtv, nullptr);
    D3D11_VIEWPORT vp{ 0, 0, (float)g_rsx.width, (float)g_rsx.height, 0, 1 };
    g_ctx11->RSSetViewports(1, &vp);
    g_ctx11->Draw(6, 0);
    static int sd = 0;
    if (sd < 12) { ps3rt_log("[rsx] XXIII-T/V HLSL textured quad draw\n"); ++sd; }
}


static void rsx_release_guest_tex() {
    if (g_guest_srv) { g_guest_srv->Release(); g_guest_srv = nullptr; }
    if (g_guest_tex) { g_guest_tex->Release(); g_guest_tex = nullptr; }
    g_guest_tex_w = g_guest_tex_h = 0;
    g_guest_tex_src = 0;
}

// V: upload A8R8G8B8 (or raw 32bpp) region from guest RAM into D3D texture
static void rsx_upload_guest_tex(uint8_t* mem) {
    // XXIII-P: pitch/format-aware guest → D3D11 upload (32bpp paths + expand 16bpp)
    if (!mem || !g_dev || !g_ctx11) return;
    uint32_t w = g_rsx.tex_width ? g_rsx.tex_width : (g_rsx.width ? g_rsx.width : 256);
    uint32_t h = g_rsx.tex_height ? g_rsx.tex_height : (g_rsx.height ? g_rsx.height : 256);
    if (w > 4096) w = 4096;
    if (h > 4096) h = 4096;
    uint64_t off = g_rsx.tex_offset ? g_rsx.tex_offset : g_rsx.color_offset;
    if (!off) return;
    uint32_t pitch = g_rsx.tex_pitch ? g_rsx.tex_pitch : (w * 4);
    // format nibble heuristics (RSX texture format field is complex; use size hints)
    uint32_t fmt = g_rsx.tex_format;
    int bpp = 4;
    bool dxt = false;
    if ((fmt & 0x9F) == 0x82 || (fmt & 0xFF) == 0x1A) bpp = 2;
    if ((fmt & 0x9F) == 0x81) bpp = 1;
    // XXIII-U: DXT/BC family (common RSX compressed enums) — expand not full yet
    if ((fmt & 0xFF) == 0x86 || (fmt & 0xFF) == 0x87 || (fmt & 0xFF) == 0x88) {
        dxt = true; bpp = 4;
        static int ud = 0;
        if (ud < 8) { ps3rt_log("[rsx] XXIII-U DXT/BC fmt=0x%x — upload as linear 32bpp fallback\n", fmt); ++ud; }
    }
    if (pitch < w * (uint32_t)bpp) pitch = w * (uint32_t)bpp;
    uint64_t need = (uint64_t)pitch * h;
    if (off >= 512ull * 1024 * 1024 || off + need > 512ull * 1024 * 1024) return;

    if (!g_guest_tex || g_guest_tex_w != w || g_guest_tex_h != h) {
        if (g_guest_tex) { g_guest_tex->Release(); g_guest_tex = nullptr; }
        D3D11_TEXTURE2D_DESC td{};
        td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DYNAMIC;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(g_dev->CreateTexture2D(&td, nullptr, &g_guest_tex)) || !g_guest_tex) return;
        g_guest_tex_w = w; g_guest_tex_h = h;
    }
    D3D11_MAPPED_SUBRESOURCE map{};
    if (FAILED(g_ctx11->Map(g_guest_tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return;
    uint8_t* dst = (uint8_t*)map.pData;
    const uint8_t* src = mem + off;
    for (uint32_t y = 0; y < h; ++y) {
        const uint8_t* row = src + (uint64_t)y * pitch;
        uint8_t* drow = dst + (uint64_t)y * map.RowPitch;
        if (bpp == 4) {
            uint32_t bytes = w * 4;
            if (bytes > map.RowPitch) bytes = map.RowPitch;
            if (bytes > pitch) bytes = pitch;
            std::memcpy(drow, row, bytes);
        } else if (bpp == 2) {
            for (uint32_t x = 0; x < w; ++x) {
                uint16_t p = (uint16_t)row[x * 2] | ((uint16_t)row[x * 2 + 1] << 8);
                // A1R5G5B5-ish expand
                uint8_t a = (p & 0x8000) ? 255 : 0;
                uint8_t r = (uint8_t)(((p >> 10) & 31) * 255 / 31);
                uint8_t g = (uint8_t)(((p >> 5) & 31) * 255 / 31);
                uint8_t b = (uint8_t)((p & 31) * 255 / 31);
                drow[x * 4 + 0] = b; drow[x * 4 + 1] = g; drow[x * 4 + 2] = r; drow[x * 4 + 3] = a;
            }
        } else {
            for (uint32_t x = 0; x < w; ++x) {
                uint8_t v = row[x];
                drow[x * 4 + 0] = v; drow[x * 4 + 1] = v; drow[x * 4 + 2] = v; drow[x * 4 + 3] = 255;
            }
        }
    }
    g_ctx11->Unmap(g_guest_tex, 0);
    g_guest_tex_src = off;
    static int su = 0;
    if (su < 16) {
        ps3rt_log("[rsx] XXIII-P upload off=0x%llx %ux%u pitch=%u fmt=0x%x bpp=%d\n",
            (unsigned long long)off, w, h, pitch, fmt, bpp);
        ++su;
    }
}

// Y: nearest-neighbor scale guest tex onto backbuffer (CPU path, no PS required)
static void rsx_blit_guest_tex_fullscreen(uint8_t* /*mem*/) {
    if (!g_guest_tex || !g_ctx11 || !g_dev || !g_swap) return;
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb) return;
    D3D11_TEXTURE2D_DESC bbd{};
    bb->GetDesc(&bbd);
    // staging for BB write
    D3D11_TEXTURE2D_DESC sd = bbd;
    sd.Usage = D3D11_USAGE_STAGING;
    sd.BindFlags = 0;
    sd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    sd.MiscFlags = 0;
    ID3D11Texture2D* staging = nullptr;
    if (FAILED(g_dev->CreateTexture2D(&sd, nullptr, &staging)) || !staging) {
        bb->Release();
        return;
    }
    // read guest tex
    D3D11_MAPPED_SUBRESOURCE srcmap{};
    if (FAILED(g_ctx11->Map(g_guest_tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &srcmap))) {
        // dynamic may not allow read — re-upload path: use color clear fallback
        staging->Release(); bb->Release();
        return;
    }
    // Actually DYNAMIC+WRITE_DISCARD invalid for read; create CPU copy via second map on STAGING copy of guest
    g_ctx11->Unmap(g_guest_tex, 0);
    D3D11_TEXTURE2D_DESC gd{};
    g_guest_tex->GetDesc(&gd);
    D3D11_TEXTURE2D_DESC gs = gd;
    gs.Usage = D3D11_USAGE_STAGING;
    gs.BindFlags = 0;
    gs.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    gs.MiscFlags = 0;
    ID3D11Texture2D* gst = nullptr;
    if (FAILED(g_dev->CreateTexture2D(&gs, nullptr, &gst)) || !gst) {
        staging->Release(); bb->Release(); return;
    }
    g_ctx11->CopyResource(gst, g_guest_tex);
    D3D11_MAPPED_SUBRESOURCE gm{}, bm{};
    if (FAILED(g_ctx11->Map(gst, 0, D3D11_MAP_READ, 0, &gm))) {
        gst->Release(); staging->Release(); bb->Release(); return;
    }
    // fill staging BB with scaled tex
    if (FAILED(g_ctx11->Map(staging, 0, D3D11_MAP_WRITE, 0, &bm))) {
        g_ctx11->Unmap(gst, 0); gst->Release(); staging->Release(); bb->Release(); return;
    }
    const uint32_t sw = g_guest_tex_w ? g_guest_tex_w : 1;
    const uint32_t sh = g_guest_tex_h ? g_guest_tex_h : 1;
    const uint32_t dw = bbd.Width, dh = bbd.Height;
    for (uint32_t y = 0; y < dh; ++y) {
        uint32_t sy = y * sh / dh;
        const uint8_t* srow = (const uint8_t*)gm.pData + sy * gm.RowPitch;
        uint8_t* drow = (uint8_t*)bm.pData + y * bm.RowPitch;
        for (uint32_t x = 0; x < dw; ++x) {
            uint32_t sx = x * sw / dw;
            const uint8_t* p = srow + sx * 4;
            drow[x*4+0] = p[0]; drow[x*4+1] = p[1]; drow[x*4+2] = p[2]; drow[x*4+3] = 255;
        }
    }
    g_ctx11->Unmap(staging, 0);
    g_ctx11->Unmap(gst, 0);
    g_ctx11->CopyResource(bb, staging);
    gst->Release(); staging->Release(); bb->Release();
    static int syb = 0;
    if (syb < 12) { ps3rt_log("[rsx] XXII-A fullscreen blit %ux%u -> %ux%u\n", sw, sh, dw, dh); ++syb; }
}



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
// Load ../PIC1.PNG (PS3_GAME when EXE is in USRDIR) for window + taskbar only.
static HICON load_pic1_png_as_hicon() {
    char mod[MAX_PATH] = {};
    if (!GetModuleFileNameA(nullptr, mod, MAX_PATH)) return nullptr;
    char* slash = strrchr(mod, '\\');
    if (!slash) return nullptr;
    *slash = 0;
    // XX-O: USRDIR -> ../PIC1.PNG (PS3_GAME); forward slashes avoid C4129
    char candidates[8][MAX_PATH];
    std::snprintf(candidates[0], MAX_PATH, "%s/../PIC1.PNG", mod);
    std::snprintf(candidates[1], MAX_PATH, "%s/../PS3_GAME/PIC1.PNG", mod);
    std::snprintf(candidates[2], MAX_PATH, "%s/PIC1.PNG", mod);
    std::snprintf(candidates[3], MAX_PATH, "%s/PS3_GAME/PIC1.PNG", mod);
    std::snprintf(candidates[4], MAX_PATH, "%s/../../PIC1.PNG", mod);
    std::snprintf(candidates[5], MAX_PATH, "%s/../../PS3_GAME/PIC1.PNG", mod);
    std::snprintf(candidates[6], MAX_PATH, "%s/PIC1.PNG", mod);
    std::snprintf(candidates[7], MAX_PATH, "%s/../PIC1.PNG", mod);
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
    if (hicon) ps3rt_log("[rsx] PIC1 window/taskbar icon from %s\n", path);
    return hicon;
}
static void apply_window_icons(HWND hwnd) {
    if (!hwnd) return;
    HICON big = load_pic1_png_as_hicon();
    if (big) {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)big);
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)big);
    }
}
#endif


// XXII-L: graphics API probe prep (D3D11 preferred; OpenGL fallback future)
static int g_gfx_api = 0; // 0=D3D11, 1=OpenGL (reserved)
static void rsx_probe_api_preference() {
    static int once = 0;
    if (once) return;
    once = 1;
    // Current build path is D3D11. Future: query adapter / Feature Level and set g_gfx_api.
    g_gfx_api = 0;
    ps3rt_log("[rsx] XXII-L API preference: D3D11 (OpenGL 3.3/2.1 fallback not linked yet)\n");
}

static void rsx_host_init(uint32_t w, uint32_t h) {
    if (g_hwnd) return;
    rsx_probe_api_preference();
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
    // XXV-FRAME: always a bright base clear so a frame is visible even without guest content
    {
        uint32_t t = (uint32_t)(g_rsx.flip_count * 5);
        uint32_t r = 40 + (t & 0x7F);
        uint32_t g = 20 + ((t * 3) & 0x5F);
        uint32_t b = 80 + ((t * 7) & 0x7F);
        if (!g_rsx.color_offset && g_rsx.draws == 0)
            g_rsx.clear_color = (r << 16) | (g << 8) | b;
    }
    uint32_t mask = g_rsx.clear_mask;
    float rr = ((g_rsx.clear_color >> 16) & 0xFF) / 255.f;
    float gg = ((g_rsx.clear_color >> 8) & 0xFF) / 255.f;
    float bb = (g_rsx.clear_color & 0xFF) / 255.f;
    float col[4] = { rr, gg, bb, 1.f };
    // VISIBLE FRAME FIX: one clear, always solid triangle, never black guest wipe
    try {
        g_ctx11->OMSetRenderTargets(1, &g_rtv, g_dsv);
        if (g_dss) g_ctx11->OMSetDepthStencilState(g_dss, g_rsx.clear_stencil);
        {
            uint32_t tck = (uint32_t)(g_rsx.flip_count * 7);
            float colb[4] = {
                (50.f + (float)(tck & 0x7F)) / 255.f,
                (80.f + (float)((tck * 3) & 0x7F)) / 255.f,
                (140.f + (float)((tck * 5) & 0x7F)) / 255.f,
                1.f
            };
            if (g_rsx.clear_color & 0x00FFFFFFu) {
                colb[0] = ((g_rsx.clear_color >> 16) & 0xFF) / 255.f;
                colb[1] = ((g_rsx.clear_color >> 8) & 0xFF) / 255.f;
                colb[2] = (g_rsx.clear_color & 0xFF) / 255.f;
            }
            g_ctx11->ClearRenderTargetView(g_rtv, colb);
            if (g_dsv && (mask & 0x02))
                g_ctx11->ClearDepthStencilView(g_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                    g_rsx.clear_depth, g_rsx.clear_stencil);
        }

        // XXV-K: ensure guest surface has pixels, then host present
        {
            uint8_t* m = ps3rt_memory();
            if (m) {
                // Always refresh synthetic frame until real shaders bind
                if (g_rsx.shader_binds == 0 || g_rsx.tex_binds == 0)
                    rsx_paint_guest_color_buffer(m);
            }
        }
        rsx_draw_solid_triangle();

        // Guest texture only if non-black samples
        {
            uint8_t* m = ps3rt_memory();
            bool guest_ok = false;
            uint32_t off = g_rsx.tex_offset ? g_rsx.tex_offset : g_rsx.color_offset;
            if (m && off && off + 256u < 0x10000000u) {
                int nz = 0;
                for (uint32_t i = 0; i < 256; i += 4) {
                    uint8_t* p = m + off + i;
                    if (p[0] | p[1] | p[2]) ++nz;
                }
                guest_ok = nz > 8;
            }
            if (guest_ok) {
                if (!g_rsx.tex_offset) g_rsx.tex_offset = g_rsx.color_offset;
                if (!g_rsx.tex_width) {
                    g_rsx.tex_width = g_rsx.width ? g_rsx.width : 1280;
                    g_rsx.tex_height = g_rsx.height ? g_rsx.height : 720;
                }
                if (!g_rsx.tex_pitch && g_rsx.tex_width)
                    g_rsx.tex_pitch = (g_rsx.tex_width * 4 + 63u) & ~63u;
                rsx_upload_guest_tex(m);
                rsx_draw_textured_quad();
            } else {
                static int pd = 0;
                if (pd < 10) {
                    ps3rt_log("[rsx] guest empty/black — solid triangle only (flip=%llu)\n",
                        (unsigned long long)g_rsx.flip_count);
                    ++pd;
                }
            }
        }

        if (g_hwnd) {
            char title[192];
            const char* gn = ps3rt_get_game_name();
            std::snprintf(title, sizeof(title), "PPSX33 | %s | flips=%llu draws=%d",
                (gn && gn[0]) ? gn : "PS3",
                (unsigned long long)g_rsx.flip_count, g_rsx.draws);
            SetWindowTextA(g_hwnd, title);
        }

        g_rsx.draws_this_frame = 0;
        g_rsx.methods_this_frame = 0;
        if (g_swap) {
            g_swap->Present(0, 0);
            static int pf = 0;
            if (pf < 20) {
                ps3rt_log("[rsx] XXV-FRAME present flip=%llu color=0x%x tex=0x%x draws=%d\n",
                    (unsigned long long)g_rsx.flip_count, g_rsx.clear_color,
                    g_rsx.tex_offset ? g_rsx.tex_offset : g_rsx.color_offset,
                    g_rsx.draws);
                ++pf;
            }
        }
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


// ============================================================================
// XXV-D: RSX pipeline skeleton (audit-aligned names)
// ============================================================================
static void rsx_surface_bind_color(uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h) {
    g_rsx.color_offset = offset;
    g_rsx.color_valid = (offset != 0);
    if (pitch) g_rsx.color_pitch = pitch;
    if (w) g_rsx.width = w;
    if (h) g_rsx.height = h;
    // Present path uses tex_* as upload source
    g_rsx.tex_offset = offset;
    if (w) g_rsx.tex_width = w;
    if (h) g_rsx.tex_height = h;
    if (pitch) g_rsx.tex_pitch = pitch;
    else if (w) g_rsx.tex_pitch = (w * 4 + 63u) & ~63u;
    static int sb = 0;
    if (sb < 12) {
        ps3rt_log("[rsx] XXV-D surface_bind color=0x%x pitch=%u %ux%u\n",
            offset, g_rsx.tex_pitch, g_rsx.tex_width, g_rsx.tex_height);
        ++sb;
    }
}


// ============================================================================
// XXV-K: paint guest color surface so present has real pixels (frame start, not FPS)
// Cell RSX color buffers are typically A8R8G8B8 or X8R8G8B8 linear in local memory.
// ============================================================================
static void rsx_paint_guest_color_buffer(uint8_t* mem) {
    if (!mem) return;
    uint32_t off = (uint32_t)(g_rsx.color_offset ? g_rsx.color_offset : g_rsx.tex_offset);
    if (!off || off >= 0x0F000000u) {
        // No surface yet — bind a synthetic host-visible region in low RSX window
        off = 0x0C000000u;
        g_rsx.color_offset = off;
        g_rsx.color_valid = true;
        g_rsx.tex_offset = off;
    }
    uint32_t w = g_rsx.tex_width ? g_rsx.tex_width : (g_rsx.width ? g_rsx.width : 1280);
    uint32_t h = g_rsx.tex_height ? g_rsx.tex_height : (g_rsx.height ? g_rsx.height : 720);
    if (w < 64) w = 1280;
    if (h < 64) h = 720;
    if (w > 1920) w = 1280;
    if (h > 1080) h = 720;
    uint32_t pitch = g_rsx.color_pitch ? g_rsx.color_pitch : ((w * 4 + 63u) & ~63u);
    g_rsx.tex_width = w;
    g_rsx.tex_height = h;
    g_rsx.tex_pitch = pitch;
    g_rsx.tex_offset = off;
    g_rsx.color_offset = off;
    g_rsx.color_valid = true;

    // Bound check against 256MB guest window
    uint64_t need = (uint64_t)off + (uint64_t)pitch * h;
    if (need > 256ull * 1024 * 1024) return;

    uint32_t cc = g_rsx.clear_color ? g_rsx.clear_color : 0x203060u;
    uint8_t cr = (uint8_t)((cc >> 16) & 0xFF);
    uint8_t cg = (uint8_t)((cc >> 8) & 0xFF);
    uint8_t cb = (uint8_t)(cc & 0xFF);
    uint32_t t = (uint32_t)(g_rsx.flip_count & 0xFF);

    // Fill gradient background + solid triangle in guest memory (BE pixel order for RSX: often BGRA on sample)
    for (uint32_t y = 0; y < h; ++y) {
        uint8_t* row = mem + off + (uint64_t)y * pitch;
        for (uint32_t x = 0; x < w; ++x) {
            // background gradient
            uint8_t r = (uint8_t)(cr / 2 + (x * 80 / w) + (t / 4));
            uint8_t g = (uint8_t)(cg / 2 + (y * 60 / h));
            uint8_t b = (uint8_t)(cb / 2 + 40 + ((x + y + t) & 0x3F));
            // triangle: barycentric-ish against three corners
            int ax = (int)w / 2, ay = (int)h / 8;
            int bx = (int)w / 8, by = (int)h * 7 / 8;
            int cx = (int)w * 7 / 8, cy = (int)h * 7 / 8;
            auto edge = [](int x0, int y0, int x1, int y1, int px, int py) {
                return (px - x0) * (y1 - y0) - (py - y0) * (x1 - x0);
            };
            int e0 = edge(ax, ay, bx, by, (int)x, (int)y);
            int e1 = edge(bx, by, cx, cy, (int)x, (int)y);
            int e2 = edge(cx, cy, ax, ay, (int)x, (int)y);
            if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0)) {
                r = (uint8_t)(220 + (t & 31));
                g = (uint8_t)(40 + (x & 63));
                b = (uint8_t)(40 + (y & 63));
            }
            // Store as B8G8R8A8 little-endian bytes (D3D-friendly upload path already treats linear)
            row[x * 4 + 0] = b;
            row[x * 4 + 1] = g;
            row[x * 4 + 2] = r;
            row[x * 4 + 3] = 0xFF;
        }
    }
    static int s_paint = 0;
    if (s_paint < 12) {
        ps3rt_log("[rsx] XXV-K paint guest color off=0x%x %ux%u pitch=%u clear=0x%x\n",
            off, w, h, pitch, cc);
        ++s_paint;
    }
}

static void rsx_present_guest_or_diag(uint8_t* mem) {
    if (!mem || !g_ctx11 || !g_rtv) return;
    if (!(g_rsx.color_valid && g_rsx.color_offset)) return;
    uint32_t off = g_rsx.color_offset;
    int nz = 0;
    if (off + 256u < 0x10000000u) {
        for (uint32_t i = 0; i < 256; i += 4) {
            uint8_t* p = mem + off + i;
            if (p[0] | p[1] | p[2]) ++nz;
        }
    }
    if (nz <= 8) return;
    g_rsx.tex_offset = g_rsx.color_offset;
    if (!g_rsx.tex_width) g_rsx.tex_width = g_rsx.width ? g_rsx.width : 1280;
    if (!g_rsx.tex_height) g_rsx.tex_height = g_rsx.height ? g_rsx.height : 720;
    if (!g_rsx.tex_pitch) g_rsx.tex_pitch = (g_rsx.tex_width * 4 + 63u) & ~63u;
    rsx_upload_guest_tex(mem);
    rsx_draw_textured_quad();
}


// XXV-D FULL DEPTH — pipeline functions (part 1: FIFO + surfaces + present)
// ============================================================================
static void rsx_sync_guest_host(uint8_t* mem) {
    if (!mem) return;
    if (g_rsx.color_valid && g_rsx.color_offset) {
        g_rsx.tex_offset = g_rsx.color_offset;
        if (!g_rsx.tex_width) g_rsx.tex_width = g_rsx.width ? g_rsx.width : 1280;
        if (!g_rsx.tex_height) g_rsx.tex_height = g_rsx.height ? g_rsx.height : 720;
        if (!g_rsx.tex_pitch) g_rsx.tex_pitch = (g_rsx.tex_width * 4 + 63u) & ~63u;
    }
}


// ============================================================================
// NV40 vertex / fragment path (frame rendering start)
// - Vertex: fetch attribute 0 (position) from guest arrays when bound
// - Vertex program bind: record ucode offset; apply identity clip-space transform
// - Fragment program bind: record ucode; output constant or texture0 sample
// Full SCE VP/FP ISA translation remains iterative; this path draws real guest verts.
// ============================================================================
static uint32_t g_va_offset[8] = {};
static uint32_t g_va_stride[8] = {};
static uint32_t g_va_size[8] = {};   // components
static uint32_t g_va_type[8] = {};   // 2=float etc
static uint32_t g_draw_count = 0;
static uint32_t g_draw_first = 0;
static uint32_t g_begin_mode = 0; // 1=points 2=lines 3=tris ...
static ID3D11Buffer* g_vb_nv40 = nullptr;
static ID3D11VertexShader* g_vs_nv40 = nullptr;
static ID3D11PixelShader* g_ps_nv40 = nullptr;
static ID3D11InputLayout* g_layout_nv40 = nullptr;
static int g_nv40_ready = 0;
static const char* kNv40HLSL = R"(
struct VSIn { float3 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut VSMain(VSIn i) {
  VSOut o;
  o.pos = float4(i.pos.xy, 0.0, 1.0);
  o.uv = i.uv;
  return o;
}
Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);
float4 PSMain(VSOut i) : SV_Target {
  // Prefer textured fragment when SRV bound; else magenta diagnostic
  float4 t = tex0.Sample(samp0, i.uv);
  if (t.a > 0.01 || any(t.rgb > 0.01)) return t;
  return float4(0.9, 0.2, 0.7, 1);
}
)";

static bool rsx_ensure_nv40_pipeline() {
    if (g_nv40_ready) return g_vs_nv40 && g_ps_nv40 && g_layout_nv40;
    g_nv40_ready = 1;
#if defined(_WIN32)
    if (!g_dev) return false;
    HMODULE d3dcomp = LoadLibraryA("d3dcompiler_47.dll");
    if (!d3dcomp) d3dcomp = LoadLibraryA("d3dcompiler_43.dll");
    if (!d3dcomp) return false;
    typedef HRESULT (WINAPI *PFN_D3DCompile)(LPCVOID, SIZE_T, LPCSTR, const void*, const void*,
        LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
    auto pCompile = (PFN_D3DCompile)GetProcAddress(d3dcomp, "D3DCompile");
    if (!pCompile) return false;
    ID3DBlob *vsb = nullptr, *psb = nullptr, *err = nullptr;
    if (FAILED(pCompile(kNv40HLSL, strlen(kNv40HLSL), "nv40", nullptr, nullptr, "VSMain", "vs_4_0", 0, 0, &vsb, &err)) || !vsb) {
        if (err) { ps3rt_log("[rsx] NV40 VS compile fail\n"); err->Release(); }
        return false;
    }
    if (FAILED(pCompile(kNv40HLSL, strlen(kNv40HLSL), "nv40", nullptr, nullptr, "PSMain", "ps_4_0", 0, 0, &psb, &err)) || !psb) {
        if (err) { err->Release(); }
        vsb->Release();
        return false;
    }
    g_dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g_vs_nv40);
    g_dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_ps_nv40);
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    g_dev->CreateInputLayout(ied, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g_layout_nv40);
    vsb->Release(); psb->Release();
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = 65536;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_dev->CreateBuffer(&bd, nullptr, &g_vb_nv40);
    ps3rt_log("[rsx] NV40 VS/PS pipeline ready (guest vertex fetch)\n");
    return g_vs_nv40 && g_ps_nv40 && g_vb_nv40;
#else
    return false;
#endif
}

static void rsx_draw_nv40_geometry(uint8_t* mem) {
#if defined(_WIN32)
    if (!mem || !g_ctx11 || !g_rtv) return;
    if (!rsx_ensure_nv40_pipeline()) {
        rsx_draw_solid_triangle();
        return;
    }
    // Prefer attribute 0 as position
    uint32_t off = g_va_offset[0];
    uint32_t stride = g_va_stride[0] ? g_va_stride[0] : 16;
    uint32_t count = g_draw_count ? g_draw_count : 3;
    if (count > 2048) count = 2048;
    if (!off || off + (uint64_t)stride * count >= 0x10000000ull) {
        // No VA — still issue solid so frame is not empty
        rsx_draw_solid_triangle();
        return;
    }
    D3D11_MAPPED_SUBRESOURCE map{};
    if (FAILED(g_ctx11->Map(g_vb_nv40, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return;
    float* dst = (float*)map.pData;
    uint32_t first = g_draw_first;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t addr = off + (first + i) * stride;
        // Guest is BE floats often for RSX vertex data on Cell — try BE then LE
        auto rd_f = [&](uint32_t a) -> float {
            if (a + 4 > 0x10000000u) return 0.f;
            uint32_t w = ((uint32_t)mem[a] << 24) | ((uint32_t)mem[a+1] << 16) |
                         ((uint32_t)mem[a+2] << 8) | (uint32_t)mem[a+3];
            float f; std::memcpy(&f, &w, 4);
            // Heuristic: if absurd, try LE
            if (f != f || f > 1e6f || f < -1e6f) {
                std::memcpy(&f, mem + a, 4);
            }
            return f;
        };
        float x = rd_f(addr + 0);
        float y = rd_f(addr + 4);
        float z = (stride >= 12) ? rd_f(addr + 8) : 0.f;
        // Normalize common 0..viewport ranges into clip
        if (x > 1.5f || y > 1.5f) {
            float vw = g_rsx.viewport_w ? (float)g_rsx.viewport_w : (float)(g_rsx.width ? g_rsx.width : 1280);
            float vh = g_rsx.viewport_h ? (float)g_rsx.viewport_h : (float)(g_rsx.height ? g_rsx.height : 720);
            x = (x / vw) * 2.f - 1.f;
            y = 1.f - (y / vh) * 2.f;
        }
        dst[i * 5 + 0] = x;
        dst[i * 5 + 1] = y;
        dst[i * 5 + 2] = z;
        dst[i * 5 + 3] = (float)i / (float)(count ? count : 1);
        dst[i * 5 + 4] = 0.5f;
    }
    g_ctx11->Unmap(g_vb_nv40, 0);

    rsx_apply_viewport_scissor();
    g_ctx11->OMSetRenderTargets(1, &g_rtv, g_dsv);
    UINT stride_b = 20, offset_b = 0;
    g_ctx11->IASetVertexBuffers(0, 1, &g_vb_nv40, &stride_b, &offset_b);
    g_ctx11->IASetInputLayout(g_layout_nv40);
    g_ctx11->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_ctx11->VSSetShader(g_vs_nv40, nullptr, 0);
    g_ctx11->PSSetShader(g_ps_nv40, nullptr, 0);
    if (g_guest_srv) {
        g_ctx11->PSSetShaderResources(0, 1, &g_guest_srv);
        if (g_samp) g_ctx11->PSSetSamplers(0, 1, &g_samp);
    }
    UINT draw_n = (count / 3) * 3;
    if (draw_n < 3) draw_n = count >= 3 ? 3 : count;
    if (draw_n >= 3) {
        g_ctx11->Draw(draw_n, 0);
        ++g_rsx.draws;
        ++g_rsx.draws_this_frame;
        static int sd = 0;
        if (sd < 16) {
            ps3rt_log("[rsx] NV40 draw verts=%u first=%u va0=0x%x stride=%u vp=0x%x fp=0x%x\n",
                draw_n, first, off, stride, g_rsx.vp_offset, g_rsx.fp_offset);
            ++sd;
        }
    } else {
        rsx_draw_solid_triangle();
    }
#else
    (void)mem;
#endif
}

static void rsx_draw_host(uint8_t* mem) {
    rsx_apply_viewport_scissor();
    rsx_sync_guest_host(mem);
    // NV40: draw guest vertices when VA0 present; else paint + solid
    if (mem && g_va_offset[0]) {
        rsx_draw_nv40_geometry(mem);
    } else {
        if (mem) rsx_paint_guest_color_buffer(mem);
        rsx_draw_solid_triangle();
        if (mem && (g_rsx.tex_offset || g_rsx.color_offset)) {
            uint32_t off = g_rsx.tex_offset ? g_rsx.tex_offset : g_rsx.color_offset;
            int nz = 0;
            if (off && off + 64u < 0x10000000u) {
                for (uint32_t i = 0; i < 64; i += 4) {
                    uint8_t* p = mem + off + i;
                    if (p[0] | p[1] | p[2]) ++nz;
                }
            }
            if (nz > 4) {
                rsx_upload_guest_tex(mem);
                rsx_draw_textured_quad();
            }
        }
        ++g_rsx.draws_this_frame;
    }
}

// Returns true if method recognized
static bool rsx_state_apply_method(uint32_t method, uint32_t arg) {
    // Surface / color
    if (method == 0x0200 || method == 0x200) {
        rsx_surface_bind_color(arg, g_rsx.color_pitch, g_rsx.width, g_rsx.height);
        return true;
    }
    if (method == 0x0204 || method == 0x204) {
        g_rsx.color_pitch = arg;
        if (g_rsx.color_offset)
            rsx_surface_bind_color(g_rsx.color_offset, arg, g_rsx.width, g_rsx.height);
        return true;
    }
    if (method == 0x0208 || method == 0x208) {
        // surface format / type
        static int sf=0; if(sf<8){ps3rt_log("[rsx] XXV-D SURFACE_FORMAT=0x%x\n", arg);++sf;}
        return true;
    }
    if (method == 0x020C || method == 0x20C) {
        g_rsx.surf_w = (arg >> 16) & 0xFFFF;
        g_rsx.surf_h = arg & 0xFFFF;
        if (g_rsx.surf_w) g_rsx.width = g_rsx.surf_w;
        if (g_rsx.surf_h) g_rsx.height = g_rsx.surf_h;
        return true;
    }
    // Clear
    if (method == 0x1D8C || method == 0x1d8c) {
        g_rsx.clear_color = arg; return true;
    }
    if (method == 0x1D90 || method == 0x1d90) {
        g_rsx.clear_mask = arg; return true;
    }
    if (method == 0x1D94 || method == 0x1d94) {
        ++g_rsx.clears;
        // XXV-K: materialize clear into guest color buffer
        {
            uint8_t* m = ps3rt_memory();
            if (m) rsx_paint_guest_color_buffer(m);
        }
        return true;
    }
    // Texture unit 0
    if (method == 0x1A00 || method == 0x1840) {
        g_rsx.tex_offset = arg; g_rsx.tex_units_bound = 1; return true;
    }
    if (method == 0x1A04 || method == 0x1844) {
        g_rsx.tex_format = arg; return true;
    }
    if (method == 0x1A08 || method == 0x1848) {
        g_rsx.tex_width = (arg >> 16) & 0xFFFF;
        g_rsx.tex_height = arg & 0xFFFF;
        if (!g_rsx.tex_width) g_rsx.tex_width = 1;
        if (!g_rsx.tex_height) g_rsx.tex_height = 1;
        return true;
    }
    if (method == 0x1A0C || method == 0x184C) {
        g_rsx.tex_pitch = arg; return true;
    }
    // Viewport / scissor
    if (method == 0x08C0) {
        g_rsx.viewport_w = (arg >> 16) ? (arg >> 16) : g_rsx.viewport_w; return true;
    }
    if (method == 0x08C4) {
        g_rsx.viewport_h = (arg >> 16) ? (arg >> 16) : g_rsx.viewport_h; return true;
    }
    if (method == 0x08D0) {
        g_rsx.scissor_x = arg & 0xFFFF; g_rsx.scissor_w = (arg >> 16) & 0xFFFF; return true;
    }
    if (method == 0x08D4) {
        g_rsx.scissor_y = arg & 0xFFFF; g_rsx.scissor_h = (arg >> 16) & 0xFFFF; return true;
    }
    // Shader binds (record only — full translate is later depth)
    if (method == 0x1D60 || method == 0x1D64 || method == 0x1D68) {
        ++g_rsx.shader_binds;
        if (method == 0x1D60) {
            g_rsx.shader_prog = arg; g_rsx.vp_offset = arg; ++g_rsx.vertex_binds;
            static int sv = 0;
            if (sv < 12) {
                ps3rt_log("[rsx] NV40 bind VP ucode @ 0x%x\n", arg);
                ++sv;
            }
        }
        if (method == 0x1D64 || method == 0x1D68) {
            g_rsx.fp_offset = arg; g_rsx.fp_control = arg; ++g_rsx.fragment_binds;
            static int sf = 0;
            if (sf < 12) {
                ps3rt_log("[rsx] NV40 bind FP ucode @ 0x%x\n", arg);
                ++sf;
            }
        }
        return true;
    }
    if (method == 0x1D6C) { g_rsx.shader_const_count = arg; return true; }
    // Vertex array
    if (method == 0x1680 || method == 0x1684 || method == 0x1688 || method == 0x1690) {
        g_rsx.vertex_array_mask |= 1u << ((method >> 2) & 7);
        ++g_rsx.vertex_binds;
        if (method == 0x1688 || method == 0x1690) g_rsx.vp_offset = arg;
        return true;
    }
    // Begin / draw
    if (method == 0x1800 || method == 0x1808 || method == 0x1814 || method == 0x1810) {
        g_begin_mode = arg;
        g_rsx.begin_mode = arg;
        if (method == 0x1814 || method == 0x1808) {
            // count or packed first/count
            if (arg > 0 && arg < 65536) g_draw_count = arg;
            else if ((arg >> 24) > 0) g_draw_count = arg & 0xFFFFFF;
            if (g_draw_count == 0) g_draw_count = 3;
        }
        ++g_rsx.draws;
        return true;
    }
    // NOP / reference / semaphore family — acknowledge
    if (method == 0x0050 || method == 0x0054 || method == 0x0060 || method == 0x0064 ||
        method == 0x0100 || method == 0x0104) {
        return true;
    }
    return false;
}

static void rsx_fifo_decode_word(uint8_t* mem, uint32_t method, uint32_t arg) {
    (void)mem;
    if (rsx_state_apply_method(method, arg) || rsx_state_apply_method_p2(method, arg)) {
        ++g_rsx.known_methods;
        ++g_rsx.methods_seen;
        ++g_rsx.methods_this_frame;
        // Issue host draw when guest issues draw-class methods
        if (method == 0x1800 || method == 0x1808 || method == 0x1814 || method == 0x1810) {
            { uint8_t* mm = mem ? mem : ps3rt_memory(); if (mm) rsx_paint_guest_color_buffer(mm); }
            rsx_draw_host(mem);
            static int dg=0;
            if (dg < 16) {
                ps3rt_log("[rsx] XXV-D draw method=0x%x arg=0x%x tex=0x%x color=0x%x\n",
                    method, arg, g_rsx.tex_offset, (unsigned)g_rsx.color_offset);
                ++dg;
            }
        }
    } else {
        ++g_rsx.unknown_methods;
        ++g_rsx.methods_seen;
        ++g_rsx.methods_this_frame;
    }
}



// ============================================================================
// XXV-D FULL DEPTH part 2 — vertex state, depth/blend, viewport present
// ============================================================================
static uint32_t g_blend_enable = 0;
static uint32_t g_depth_test = 0;
static uint32_t g_depth_mask = 1;
static uint32_t g_color_mask = 0xF;

static void rsx_apply_viewport_scissor() {
#if defined(_WIN32)
    if (!g_ctx11) return;
    float vw = (float)(g_rsx.viewport_w ? g_rsx.viewport_w : (g_rsx.width ? g_rsx.width : 1280));
    float vh = (float)(g_rsx.viewport_h ? g_rsx.viewport_h : (g_rsx.height ? g_rsx.height : 720));
    D3D11_VIEWPORT vp{ 0.f, 0.f, vw, vh, 0.f, 1.f };
    g_ctx11->RSSetViewports(1, &vp);
#endif
}

// Minimal vertex-fetch note: record array offsets for later real geometry
static void rsx_vertex_fetch_note(uint32_t slot, uint32_t offset, uint32_t stride) {
    if (slot < 8) {
        g_va_offset[slot] = offset;
        g_va_stride[slot] = stride;
    }
    static int vn = 0;
    if (vn < 12) {
        ps3rt_log("[rsx] XXV-D-p2 vertex_fetch slot=%u off=0x%x stride=%u\n", slot, offset, stride);
        ++vn;
    }
}

static bool rsx_state_apply_method_p2(uint32_t method, uint32_t arg) {
    // Depth / stencil / blend (record + light host state)
    if (method == 0x0A00 || method == 0x0A04) { // depth test enable / mask family
        g_depth_test = arg & 1;
        if (method == 0x0A04) g_depth_mask = arg & 1;
        return true;
    }
    if (method == 0x0300 || method == 0x0304) {
        g_blend_enable = arg & 1;
        return true;
    }
    if (method == 0x0310 || method == 0x0314) {
        g_color_mask = arg;
        return true;
    }
    // Vertex attribute offsets (NV40-ish array)
    if (method >= 0x1680 && method <= 0x16BC && ((method & 3) == 0)) {
        uint32_t slot = (method - 0x1680) / 4;
        if (slot < 8) {
            rsx_vertex_fetch_note(slot, arg, g_va_stride[slot] ? g_va_stride[slot] : 12);
            g_rsx.vertex_array_mask |= (1u << slot);
            ++g_rsx.vertex_binds;
        }
        return true;
    }
    // Z-offset / depth surface
    if (method == 0x0210 || method == 0x210) {
        static int zd=0; if(zd<8){ps3rt_log("[rsx] XXV-D-p2 Z_OFFSET=0x%x\n", arg);++zd;}
        return true;
    }
    if (method == 0x0214 || method == 0x214) {
        static int zp=0; if(zp<8){ps3rt_log("[rsx] XXV-D-p2 Z_PITCH=0x%x\n", arg);++zp;}
        return true;
    }
    // Semaphore / report
    if (method == 0x1D00 || method == 0x1D04 || method == 0x1D10) {
        return true;
    }
    return false;
}

static void rsx_process_fifo(uint8_t* mem) {
    if (!mem || !g_rsx.fifo_addr || !g_rsx.fifo_size) return;
    uint32_t put = g_rsx.put % g_rsx.fifo_size;
    uint32_t get = g_rsx.get % g_rsx.fifo_size;
    // XXIV-B: no drain when empty or only tiny residual (e.g. put=get=0x20)
    if (put == get) return;
    uint32_t delta = (put >= get) ? (put - get) : (g_rsx.fifo_size - get + put);
    if (delta < 8) return; // less than one method word pair
    int steps = 0;
    const int kMaxSteps = (int)((delta / 8) + 64);
    while (get != put && steps < kMaxSteps && steps < 8192) { // XXV-D depth
        uint64_t fa = g_rsx.fifo_addr + get;
        if (fa + 8 > 512ull * 1024 * 1024) break;
        uint32_t hdr = (mem[fa] << 24) | (mem[fa+1] << 16) | (mem[fa+2] << 8) | mem[fa+3];
        uint32_t arg = (mem[fa+4] << 24) | (mem[fa+5] << 16) | (mem[fa+6] << 8) | mem[fa+7];
        uint32_t method = hdr & 0x1FFC;
        // JUMP / CALL style
        if ((hdr & 0xE0000003u) == 0x20000000u) {
            uint32_t target = arg % (g_rsx.fifo_size ? g_rsx.fifo_size : 1);
            get = target;
            ++steps;
            static int jn = 0;
            if (jn < 8) { ps3rt_log("[rsx] XXV-D fifo jump -> 0x%x\n", target); ++jn; }
            continue;
        }
        // Full method apply + host draw on draw methods
        rsx_fifo_decode_word(mem, method, arg);
        // Also accept alternate clear/color encodings not in apply table
        if (method == 0x01D8 || method == 0x1D8) {
            g_rsx.clear_color = arg & 0xFFFFFF;
            ++g_rsx.clears;
        }
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
    rsx_ensure_hlsl(); // XXIV-C prove HLSL at init
    ps3rt_log("[rsx] init fifo=0x%llx size=0x%x backend=%d\n", (unsigned long long)fifo_addr, g_rsx.fifo_size, g_backend);
}

PS3RT_API void ps3rt_rsx_set_color_surface(uint32_t offset, uint32_t w, uint32_t h) {
    rsx_surface_bind_color(offset, w ? ((w*4+63)&~63u) : 0, w, h);
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
        ps3rt_log("[rsx] XXV-E flip #%llu draws=%d clears=%d depth=%d stencil=%d shaders=%d tex=%d methods=%d known=%d unknown=%d %ux%u\n",
            (unsigned long long)g_rsx.flip_count, g_rsx.draws, g_rsx.clears,
            g_rsx.depth_clears, g_rsx.stencil_clears,
            g_rsx.shader_binds, g_rsx.tex_binds + g_rsx.tex_units_bound,
            g_rsx.methods_this_frame, g_rsx.known_methods, g_rsx.unknown_methods,
            g_rsx.width, g_rsx.height);
    }
}

PS3RT_API uint64_t ps3rt_rsx_flip_count(void) {
    return g_rsx.flip_count;
}
