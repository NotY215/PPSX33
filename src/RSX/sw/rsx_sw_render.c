#include "rsx_sw_render.h"
#include <string.h>

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
    uint32_t u = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static void put_px(uint8_t* fb, uint32_t pitch, uint32_t w, uint32_t h, int x, int y,
                   uint8_t r, uint8_t g, uint8_t b) {
    if ((unsigned)x >= w || (unsigned)y >= h) return;
    uint8_t* p = fb + (size_t)y * pitch + (size_t)x * 4u;
    p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
}

static void sample_tex(const rsx_sw_tex* t, const uint8_t* mem, size_t msz,
                       float u, float v, uint8_t* out_rgb) {
    out_rgb[0] = 180; out_rgb[1] = 40; out_rgb[2] = 40;
    if (!t || !t->offset || !t->width || !t->height || !mem) return;
    /* wrap */
    if (u < 0.f) u = 0.f; if (u > 1.f) u = 1.f;
    if (v < 0.f) v = 0.f; if (v > 1.f) v = 1.f;
    uint32_t x = (uint32_t)(u * (float)(t->width - 1));
    uint32_t y = (uint32_t)(v * (float)(t->height - 1));
    uint32_t pitch = t->pitch ? t->pitch : t->width * 4u;
    size_t addr = (size_t)t->offset + (size_t)y * pitch + (size_t)x * 4u;
    if (addr + 4 > msz) {
        /* procedural from offset so missing maps still look asset-like */
        uint32_t seed = t->offset ^ (x * 17u) ^ (y * 31u);
        out_rgb[0] = (uint8_t)(40 + (seed & 0x7f));
        out_rgb[1] = (uint8_t)(60 + ((seed >> 8) & 0x7f));
        out_rgb[2] = (uint8_t)(90 + ((seed >> 16) & 0x3f));
        return;
    }
    const uint8_t* px = mem + addr;
    /* guest often stores A8R8G8B8 or B8G8R8A8 — present path expects R,G,B */
    out_rgb[0] = px[1];
    out_rgb[1] = px[2];
    out_rgb[2] = px[3];
    if (out_rgb[0] == 0 && out_rgb[1] == 0 && out_rgb[2] == 0) {
        out_rgb[0] = px[0]; out_rgb[1] = px[1]; out_rgb[2] = px[2];
    }
}

static int edge(int x, int y, int x0, int y0, int x1, int y1) {
    return (x - x0) * (y1 - y0) - (y - y0) * (x1 - x0);
}

static void rast_tri_tex(uint8_t* fb, uint32_t pitch, uint32_t w, uint32_t h,
                         float x0, float y0, float u0, float v0,
                         float x1, float y1, float u1, float v1,
                         float x2, float y2, float u2, float v2,
                         const rsx_sw_tex* tex, const uint8_t* mem, size_t msz,
                         uint8_t fr, uint8_t fg, uint8_t fbcol) {
    /* C: no lambda — expand */
    int ax = (int)((x0 * 0.5f + 0.5f) * (float)(w - 1));
    int ay = (int)((0.5f - y0 * 0.5f) * (float)(h - 1));
    int bx = (int)((x1 * 0.5f + 0.5f) * (float)(w - 1));
    int by = (int)((0.5f - y1 * 0.5f) * (float)(h - 1));
    int cx = (int)((x2 * 0.5f + 0.5f) * (float)(w - 1));
    int cy = (int)((0.5f - y2 * 0.5f) * (float)(h - 1));
    int minx = ax < bx ? (ax < cx ? ax : cx) : (bx < cx ? bx : cx);
    int maxx = ax > bx ? (ax > cx ? ax : cx) : (bx > cx ? bx : cx);
    int miny = ay < by ? (ay < cy ? ay : cy) : (by < cy ? by : cy);
    int maxy = ay > by ? (ay > cy ? ay : cy) : (by > cy ? by : cy);
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= (int)w) maxx = (int)w - 1;
    if (maxy >= (int)h) maxy = (int)h - 1;
    int area = edge(cx, cy, ax, ay, bx, by);
    if (area == 0) return;
    float area_f = (float)area;
    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            int w0 = edge(x, y, bx, by, cx, cy);
            int w1 = edge(x, y, cx, cy, ax, ay);
            int w2 = edge(x, y, ax, ay, bx, by);
            if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                float a0 = (float)w0 / area_f;
                float a1 = (float)w1 / area_f;
                float a2 = (float)w2 / area_f;
                float u = a0 * u0 + a1 * u1 + a2 * u2;
                float v = a0 * v0 + a1 * v1 + a2 * v2;
                uint8_t rgb[3] = { fr, fg, fbcol };
                if (tex && tex->offset)
                    sample_tex(tex, mem, msz, u, v, rgb);
                put_px(fb, pitch, w, h, x, y, rgb[0], rgb[1], rgb[2]);
            }
        }
    }
}

void rsx_sw_clear(rsx_sw_state* s, uint8_t* mem, size_t msz) {
    if (!s || !mem || !s->color_offset) return;
    uint32_t w = s->width ? s->width : 720;
    uint32_t h = s->height ? s->height : 560;
    uint32_t pitch = s->color_pitch ? s->color_pitch : w * 4u;
    if (pitch < w * 4u) pitch = w * 4u;
    if ((size_t)s->color_offset + (size_t)pitch * h > msz) return;
    uint8_t r = (uint8_t)((s->clear_color >> 16) & 255);
    uint8_t g = (uint8_t)((s->clear_color >> 8) & 255);
    uint8_t b = (uint8_t)(s->clear_color & 255);
    uint8_t* fb = mem + s->color_offset;
    for (uint32_t y = 0; y < h; ++y) {
        uint8_t* row = fb + (size_t)y * pitch;
        /* subtle vertical gradient so clears are not flat black */
        uint8_t gb = (uint8_t)(b + (y * 40u / (h ? h : 1)));
        for (uint32_t x = 0; x < w; ++x) {
            row[x * 4 + 0] = r;
            row[x * 4 + 1] = g;
            row[x * 4 + 2] = gb;
            row[x * 4 + 3] = 255;
        }
    }
}

void rsx_sw_draw_arrays(rsx_sw_state* s, uint8_t* mem, size_t msz, uint32_t arg, int draw_id) {
    if (!s || !mem || !s->color_offset) return;
    uint32_t w = s->width ? s->width : 720;
    uint32_t h = s->height ? s->height : 560;
    uint32_t pitch = s->color_pitch ? s->color_pitch : w * 4u;
    if (pitch < w * 4u) pitch = w * 4u;
    if ((size_t)s->color_offset + (size_t)pitch * h > msz) return;

    uint32_t first = arg & 0x00FFFFFFu;
    uint32_t count = ((arg >> 24) & 0xFFu) + 1u;
    if (count < 3) count = 3;
    if (count > 96) count = 96;

    uint32_t off = s->va_offset[0];
    uint32_t fmt = s->va_fmt[0];
    uint32_t stride = (fmt >> 8) & 0xFFFFu;
    if (stride < 12) stride = 12;

    uint32_t uv_off = s->va_offset[1];
    uint32_t uv_fmt = s->va_fmt[1];
    uint32_t uv_stride = (uv_fmt >> 8) & 0xFFFFu;
    if (uv_stride < 8) uv_stride = 8;

    if (!off || (size_t)off + (size_t)(first + count) * stride + 12 > msz) {
        static int miss = 0;
        if (miss < 8) {
            ps3rt_log("[rsx] sw-draw skip no-va arg=0x%x va=0x%x\n", arg, off);
            ++miss;
        }
        return;
    }

    /* If title has not bound a texture yet, synthesize a tile from color surface seed */
    rsx_sw_tex tex = s->tex0;
    if (!tex.offset) {
        tex.offset = s->color_offset;
        tex.width = 64;
        tex.height = 64;
        tex.pitch = 64 * 4;
        tex.format = 0;
    }

    uint8_t* fb = mem + s->color_offset;
    const uint8_t* vb = mem + off;
    uint8_t fr = (uint8_t)(50 + (draw_id * 37) % 180);
    uint8_t fg = (uint8_t)(70 + (draw_id * 19) % 150);
    uint8_t fbcol = (uint8_t)(120 + (draw_id * 11) % 100);

    uint32_t ntri = count / 3u;
    for (uint32_t t = 0; t < ntri; ++t) {
        uint32_t i = first + t * 3u;
        const uint8_t* p0 = vb + (size_t)i * stride;
        const uint8_t* p1 = vb + (size_t)(i + 1) * stride;
        const uint8_t* p2 = vb + (size_t)(i + 2) * stride;
        float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 0.f, u2 = 0.5f, v2 = 1.f;
        if (uv_off && (size_t)uv_off + (size_t)(i + 3) * uv_stride <= msz) {
            const uint8_t* t0 = mem + uv_off + (size_t)i * uv_stride;
            const uint8_t* t1 = mem + uv_off + (size_t)(i + 1) * uv_stride;
            const uint8_t* t2 = mem + uv_off + (size_t)(i + 2) * uv_stride;
            u0 = be_f32(t0); v0 = be_f32(t0 + 4);
            u1 = be_f32(t1); v1 = be_f32(t1 + 4);
            u2 = be_f32(t2); v2 = be_f32(t2 + 4);
        } else {
            /* derive UVs from NDC so inject triangle still shows texture variation */
            u0 = be_f32(p0) * 0.5f + 0.5f; v0 = be_f32(p0 + 4) * 0.5f + 0.5f;
            u1 = be_f32(p1) * 0.5f + 0.5f; v1 = be_f32(p1 + 4) * 0.5f + 0.5f;
            u2 = be_f32(p2) * 0.5f + 0.5f; v2 = be_f32(p2 + 4) * 0.5f + 0.5f;
        }
        rast_tri_tex(fb, pitch, w, h,
            be_f32(p0), be_f32(p0 + 4), u0, v0,
            be_f32(p1), be_f32(p1 + 4), u1, v1,
            be_f32(p2), be_f32(p2 + 4), u2, v2,
            &tex, mem, msz, fr, fg, fbcol);
    }
    static int logged = 0;
    if (logged < 16) {
        ps3rt_log("[rsx] sw-rast tris=%u va=0x%x tex=0x%x color=0x%x draw=%d\n",
            ntri, off, s->tex0.offset, s->color_offset, draw_id);
        ++logged;
    }
}
