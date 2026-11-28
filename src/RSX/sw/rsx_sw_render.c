#include "rsx_sw_render.h"
#include <string.h>
#include <math.h>

extern void ps3rt_log(const char* fmt, ...);

void rsx_sw_state_init(rsx_sw_state* s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->width = 720;
    s->height = 560;
    s->color_pitch = 720 * 4;
    s->clear_color = 0xFF102030u;
    s->begin_mode = 4;
}

void rsx_sw_set_color(rsx_sw_state* s, uint32_t off, uint32_t pitch, uint32_t w, uint32_t h) {
    if (!s) return;
    if (off) s->color_offset = off;
    if (pitch) s->color_pitch = pitch;
    if (w) s->width = w;
    if (h) s->height = h;
}

void rsx_sw_set_tex0(rsx_sw_state* s, uint32_t off, uint32_t pitch, uint32_t w, uint32_t h, uint32_t fmt) {
    if (!s) return;
    s->tex0.offset = off;
    s->tex0.pitch = pitch ? pitch : (w ? w * 4u : 0);
    s->tex0.width = w ? w : 256;
    s->tex0.height = h ? h : 256;
    s->tex0.format = fmt;
}

void rsx_sw_set_va(rsx_sw_state* s, int idx, uint32_t off, uint32_t fmt) {
    if (!s || idx < 0 || idx >= 8) return;
    if (off) s->va_offset[idx] = off;
    s->va_fmt[idx] = fmt;
}

static float be_f32(const uint8_t* p) {
    uint32_t u = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                 ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static void put_px(uint8_t* fb, uint32_t pitch, uint32_t w, uint32_t h,
                   int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if ((unsigned)x >= w || (unsigned)y >= h) return;
    uint8_t* p = fb + (size_t)y * pitch + (size_t)x * 4u;
    p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
}

static void sample_tex(const rsx_sw_tex* t, const uint8_t* mem, size_t msz,
                       float u, float v, uint8_t* out_rgb) {
    /* default procedural base (visible when tex unbound) */
    out_rgb[0] = 48; out_rgb[1] = 96; out_rgb[2] = 160;
    if (!t || !mem) return;
    if (u < 0.f) u -= (float)(int)u - 1.f;
    if (v < 0.f) v -= (float)(int)v - 1.f;
    u = u - (float)(int)u;
    v = v - (float)(int)v;
    if (u < 0.f) u += 1.f;
    if (v < 0.f) v += 1.f;

    if (t->offset && t->width && t->height) {
        uint32_t tw = t->width, th = t->height;
        uint32_t x = (uint32_t)(u * (float)(tw > 1 ? tw - 1 : 1));
        uint32_t y = (uint32_t)(v * (float)(th > 1 ? th - 1 : 1));
        uint32_t pitch = t->pitch ? t->pitch : tw * 4u;
        size_t addr = (size_t)t->offset + (size_t)y * pitch + (size_t)x * 4u;
        if (addr + 4 <= msz) {
            const uint8_t* px = mem + addr;
            /* guest linear often BGRA or RGBA; accept either */
            out_rgb[0] = px[0];
            out_rgb[1] = px[1];
            out_rgb[2] = px[2];
            if ((out_rgb[0] | out_rgb[1] | out_rgb[2]) == 0) {
                out_rgb[0] = px[2]; out_rgb[1] = px[1]; out_rgb[2] = px[0];
            }
            return;
        }
    }
    /* procedural tile when tex unbound — gradient + checker so frames are visible */
    {
        uint32_t ix = (uint32_t)(u * 64.f) & 63u;
        uint32_t iy = (uint32_t)(v * 64.f) & 63u;
        uint32_t seed = (t ? t->offset : 0u) ^ (ix * 17u) ^ (iy * 31u);
        uint8_t base_r = (uint8_t)(40 + (seed & 0x3f));
        uint8_t base_g = (uint8_t)(60 + ((seed >> 6) & 0x3f));
        uint8_t base_b = (uint8_t)(100 + ((seed >> 12) & 0x3f));
        int checker = ((ix >> 3) ^ (iy >> 3)) & 1;
        if (checker) {
            out_rgb[0] = (uint8_t)(base_r + 40);
            out_rgb[1] = (uint8_t)(base_g + 20);
            out_rgb[2] = (uint8_t)(base_b);
        } else {
            out_rgb[0] = base_r;
            out_rgb[1] = base_g;
            out_rgb[2] = base_b;
        }
    }
}

void rsx_sw_clear(rsx_sw_state* s, uint8_t* mem, size_t msz) {
    if (!s || !mem || !s->color_offset) return;
    uint32_t w = s->width ? s->width : 720;
    uint32_t h = s->height ? s->height : 560;
    uint32_t pitch = s->color_pitch ? s->color_pitch : (w * 4u);
    size_t need = (size_t)s->color_offset + (size_t)pitch * h;
    if (need > msz || s->color_offset >= msz) return;
    uint8_t* fb = mem + s->color_offset;
    uint8_t r = (uint8_t)((s->clear_color >> 16) & 0xff);
    uint8_t g = (uint8_t)((s->clear_color >> 8) & 0xff);
    uint8_t b = (uint8_t)(s->clear_color & 0xff);
    if ((s->clear_color & 0xffffffu) == 0) {
        r = 16; g = 24; b = 40;
    }
    for (uint32_t y = 0; y < h; ++y) {
        uint8_t* row = fb + (size_t)y * pitch;
        for (uint32_t x = 0; x < w; ++x) {
            uint8_t* p = row + (size_t)x * 4u;
            /* mild vertical gradient so empty frames are not a flat void */
            p[0] = (uint8_t)(r + (y * 20u / (h ? h : 1)));
            p[1] = (uint8_t)(g + (y * 10u / (h ? h : 1)));
            p[2] = (uint8_t)(b + (x * 15u / (w ? w : 1)));
            p[3] = 255;
        }
    }
}

typedef struct { float x, y, z, u, v; } sw_vert;

static int fetch_vert(const rsx_sw_state* s, const uint8_t* mem, size_t msz,
                      uint32_t index, sw_vert* out) {
    if (!s || !mem || !out) return 0;
    uint32_t va0 = s->va_offset[0];
    if (!va0) return 0;
    /* bit31 of array OFFSET: 1 = MAIN/IO, 0 = LOCAL/VRAM — strip flag for host VA */
    va0 &= 0x7fffffffu;
    /* assume tightly packed float3 position (12 bytes) in BE */
    size_t stride = 12;
    uint32_t fmt = s->va_fmt[0];
    if ((fmt & 0x7u) == 2) stride = 16; /* float4 */
    size_t base = (size_t)va0 + (size_t)index * stride;
    if (base + 12 > msz) return 0;
    const uint8_t* p = mem + base;
    float X = be_f32(p + 0);
    float Y = be_f32(p + 4);
    float Z = be_f32(p + 8);
    /* clip / NDC → screen (PS3 often sends already projected-ish coords) */
    float sx, sy;
    if (X >= -2.f && X <= 2.f && Y >= -2.f && Y <= 2.f) {
        sx = (X * 0.5f + 0.5f) * (float)(s->width ? s->width : 720);
        sy = (1.f - (Y * 0.5f + 0.5f)) * (float)(s->height ? s->height : 560);
    } else {
        sx = X;
        sy = Y;
        if (sx > 4096.f || sy > 4096.f) {
            sx *= (float)(s->width ? s->width : 720) / 4096.f;
            sy *= (float)(s->height ? s->height : 560) / 4096.f;
        }
    }
    out->x = sx; out->y = sy; out->z = Z;
    out->u = 0.5f; out->v = 0.5f;
    /* optional UV from attribute 1 (float2/float3) */
    if (s->va_offset[1]) {
        size_t ub = (size_t)s->va_offset[1] + (size_t)index * 8u;
        if (ub + 8 <= msz) {
            out->u = be_f32(mem + ub);
            out->v = be_f32(mem + ub + 4);
        }
    } else {
        out->u = (sx / (float)(s->width ? s->width : 720));
        out->v = (sy / (float)(s->height ? s->height : 560));
    }
    return 1;
}

static float edge(float ax, float ay, float bx, float by, float cx, float cy) {
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

static void fill_tri(rsx_sw_state* s, uint8_t* mem, size_t msz,
                     const sw_vert* a, const sw_vert* b, const sw_vert* c) {
    if (!s || !mem || !s->color_offset) return;
    uint32_t w = s->width ? s->width : 720;
    uint32_t h = s->height ? s->height : 560;
    uint32_t pitch = s->color_pitch ? s->color_pitch : (w * 4u);
    size_t need = (size_t)s->color_offset + (size_t)pitch * h;
    if (need > msz) return;
    uint8_t* fb = mem + s->color_offset;

    int minx = (int)fminf(a->x, fminf(b->x, c->x));
    int maxx = (int)fmaxf(a->x, fmaxf(b->x, c->x));
    int miny = (int)fminf(a->y, fminf(b->y, c->y));
    int maxy = (int)fmaxf(a->y, fmaxf(b->y, c->y));
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= (int)w) maxx = (int)w - 1;
    if (maxy >= (int)h) maxy = (int)h - 1;
    if (minx > maxx || miny > maxy) return;

    float area = edge(a->x, a->y, b->x, b->y, c->x, c->y);
    if (area > -1e-4f && area < 1e-4f) return;
    float inv = 1.f / area;

    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float w0 = edge(b->x, b->y, c->x, c->y, px, py) * inv;
            float w1 = edge(c->x, c->y, a->x, a->y, px, py) * inv;
            float w2 = edge(a->x, a->y, b->x, b->y, px, py) * inv;
            if (w0 < 0.f || w1 < 0.f || w2 < 0.f) continue;
            float u = w0 * a->u + w1 * b->u + w2 * c->u;
            float v = w0 * a->v + w1 * b->v + w2 * c->v;
            uint8_t rgb[3];
            sample_tex(&s->tex0, mem, msz, u, v, rgb);
            put_px(fb, pitch, w, h, x, y, rgb[0], rgb[1], rgb[2]);
        }
    }
}

void rsx_sw_draw_arrays(rsx_sw_state* s, uint8_t* mem, size_t msz, uint32_t arg, int draw_id) {
    if (!s || !mem || !s->color_offset) return;
    if (!s->va_offset[0]) return;

    uint32_t count = ((arg >> 24) & 0xffu) + 1u;
    uint32_t first = arg & 0x00ffffffu;
    if (count < 3) count = 3;
    if (count > 256) count = 256;

    int mode = s->begin_mode;
    /* 4 = triangles, 5 = triangle strip, 6 = triangle fan (GCM-ish) */
    int tris = 0;
    if (mode == 5) {
        sw_vert prev0, prev1, cur;
        if (!fetch_vert(s, mem, msz, first, &prev0)) return;
        if (!fetch_vert(s, mem, msz, first + 1, &prev1)) return;
        for (uint32_t i = 2; i < count; ++i) {
            if (!fetch_vert(s, mem, msz, first + i, &cur)) break;
            if ((i & 1u) == 0u)
                fill_tri(s, mem, msz, &prev0, &prev1, &cur);
            else
                fill_tri(s, mem, msz, &prev1, &prev0, &cur);
            prev0 = prev1; prev1 = cur;
            ++tris;
        }
    } else if (mode == 6) {
        sw_vert hub, prev, cur;
        if (!fetch_vert(s, mem, msz, first, &hub)) return;
        if (!fetch_vert(s, mem, msz, first + 1, &prev)) return;
        for (uint32_t i = 2; i < count; ++i) {
            if (!fetch_vert(s, mem, msz, first + i, &cur)) break;
            fill_tri(s, mem, msz, &hub, &prev, &cur);
            prev = cur;
            ++tris;
        }
    } else {
        for (uint32_t i = 0; i + 2 < count; i += 3) {
            sw_vert a, b, c;
            if (!fetch_vert(s, mem, msz, first + i, &a)) break;
            if (!fetch_vert(s, mem, msz, first + i + 1, &b)) break;
            if (!fetch_vert(s, mem, msz, first + i + 2, &c)) break;
            fill_tri(s, mem, msz, &a, &b, &c);
            ++tris;
        }
    }

    static int slog = 0;
    if (slog < 16) {
        ps3rt_log("[rsx] sw-rast draw_id=%d mode=%d first=%u count=%u tris=%d va0=0x%x tex=0x%x color=0x%x\n",
                  draw_id, mode, first, count, tris,
                  (unsigned)s->va_offset[0], (unsigned)s->tex0.offset, (unsigned)s->color_offset);
        ++slog;
    }
}
