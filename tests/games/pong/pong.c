/*
 * PPSX33 PS3-only Pong integration sample.
 * Target: PowerPC64 big-endian PPU, PSL1GHT, RSX scan-out and VMX.
 * This is not a desktop-portable program.
 */
#include <ppu-types.h>
#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <sysutil/video.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>
#include <altivec.h>
#include <stdint.h>
#include <string.h>

#define MAX_FRAMES 2400
#define PADDLE_H 72
#define PADDLE_W 12
#define BALL_SIZE 10

static gcmContextData *g_ctx;
static u32 *g_fb[2];
static u32 g_offset[2];
static u32 g_width, g_height, g_pitch;
static int g_front;

static inline void pixel(u32 *fb, int x, int y, u32 color) {
    if (x >= 0 && y >= 0 && (u32)x < g_width && (u32)y < g_height)
        fb[(u32)y * (g_pitch / 4) + (u32)x] = color;
}
static void rect(u32 *fb, int x, int y, int w, int h, u32 color) {
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            pixel(fb, xx, yy, color);
}
static void line(u32 *fb, int x, int y, int h, u32 color) {
    for (int yy = y; yy < y + h; yy += 20)
        rect(fb, (int)g_width / 2 - 2, yy, 4, 10, color);
}
static void wait_flip(void) {
    while (gcmGetFlipStatus() != 0) sysUsleep(200);
}
static void present(void) {
    gcmSetFlip(g_ctx, g_front);
    rsxFlushBuffer(g_ctx);
    gcmResetFlipStatus();
    wait_flip();
    g_front ^= 1;
}
static void vmx_probe(void) {
    vector unsigned int a = {1, 2, 3, 4};
    vector unsigned int b = {5, 6, 7, 8};
    vector unsigned int c = vec_add(a, b);
    volatile u32 out[4] __attribute__((aligned(16)));
    vec_st(c, 0, out);
}

int main(void) {
    videoState state;
    videoResolution resolution;
    memset(&state, 0, sizeof(state));
    if (videoGetState(0, 0, &state) != 0 || state.state != VIDEO_STATE_ENABLED)
        return 1;
    if (videoGetResolution(state.displayMode.resolution, &resolution) != 0)
        return 1;
    g_width = resolution.width;
    g_height = resolution.height;
    g_pitch = g_width * 4;

    void *host_addr = NULL;
    g_ctx = rsxInit(&host_addr, 0x100000, 0x100000);
    if (!g_ctx) return 1;

    for (int i = 0; i < 2; ++i) {
        g_fb[i] = (u32 *)rsxMemalign(64, g_pitch * g_height);
        if (!g_fb[i] || rsxAddressToOffset(g_fb[i], &g_offset[i]) != 0)
            return 1;
        memset(g_fb[i], 0, g_pitch * g_height);
        gcmSetDisplayBuffer(i, g_offset[i], g_pitch, g_width, g_height);
    }
    gcmSetFlipMode(GCM_FLIP_VSYNC);
    ioPadInit(7);
    vmx_probe();

    int bx = (int)g_width / 2, by = (int)g_height / 2;
    int vx = 5, vy = 4;
    int lp = (int)g_height / 2, rp = (int)g_height / 2;
    unsigned left_score = 0, right_score = 0;

    for (unsigned frame = 0; frame < MAX_FRAMES; ++frame) {
        ioPadData pad;
        memset(&pad, 0, sizeof(pad));
        ioPadGetData(0, &pad);
        if (pad.button[0] & (1u << 4)) lp -= 7;
        if (pad.button[0] & (1u << 6)) lp += 7;
        if (lp < PADDLE_H / 2) lp = PADDLE_H / 2;
        if (lp > (int)g_height - PADDLE_H / 2) lp = (int)g_height - PADDLE_H / 2;

        if (rp < by) rp += 3;
        if (rp > by) rp -= 3;
        if (rp < PADDLE_H / 2) rp = PADDLE_H / 2;
        if (rp > (int)g_height - PADDLE_H / 2) rp = (int)g_height - PADDLE_H / 2;

        bx += vx; by += vy;
        if (by <= BALL_SIZE || by >= (int)g_height - BALL_SIZE) vy = -vy;
        if (bx <= 36 && by >= lp - PADDLE_H / 2 && by <= lp + PADDLE_H / 2) vx = 5;
        if (bx >= (int)g_width - 36 && by >= rp - PADDLE_H / 2 && by <= rp + PADDLE_H / 2) vx = -5;
        if (bx < 0) { ++right_score; bx = (int)g_width / 2; by = (int)g_height / 2; vx = 5; }
        if (bx >= (int)g_width) { ++left_score; bx = (int)g_width / 2; by = (int)g_height / 2; vx = -5; }

        u32 *fb = g_fb[g_front];
        for (u32 i = 0; i < g_width * g_height; ++i) fb[i] = 0xff071426;
        line(fb, 0, 0, (int)g_height, 0xff31506a);
        rect(fb, 24, lp - PADDLE_H / 2, PADDLE_W, PADDLE_H, 0xff22c7ff);
        rect(fb, (int)g_width - 36, rp - PADDLE_H / 2, PADDLE_W, PADDLE_H, 0xffff5c85);
        rect(fb, bx - BALL_SIZE / 2, by - BALL_SIZE / 2, BALL_SIZE, BALL_SIZE, 0xffffffff);
        present();
        sysUtilCheckCallback();
    }

    ioPadEnd();
    rsxFinish(g_ctx, 1);
    return (left_score + right_score) == 0 ? 0 : 0;
}
