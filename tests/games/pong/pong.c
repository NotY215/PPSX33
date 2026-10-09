/*
 * PPSX33 PS3-only Pong integration test.
 * Target: PowerPC64 big-endian PPU using PSL1GHT.
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

#define WIDTH 40
#define HEIGHT 24
#define FRAME_LIMIT 1200

static void vmx_probe(void) {
    vector unsigned int a = {1, 2, 3, 4};
    vector unsigned int b = {5, 6, 7, 8};
    vector unsigned int c = vec_add(a, b);
    volatile u32 out[4] __attribute__((aligned(16)));
    vec_st(c, 0, out);
}

int main(void) {
    int ball_x = WIDTH / 2, ball_y = HEIGHT / 2;
    int vx = 1, vy = 1, left_y = HEIGHT / 2, right_y = HEIGHT / 2;
    unsigned left_score = 0, right_score = 0;
    void *host_addr = NULL;
    gcmContextData *context = rsxInit(&host_addr, 0x100000, 0x100000);
    if (!context) return 1;
    ioPadInit(7);
    vmx_probe();

    for (unsigned frame = 0; frame < FRAME_LIMIT; ++frame) {
        ioPadData pad;
        memset(&pad, 0, sizeof(pad));
        ioPadGetData(0, &pad);
        if (pad.button[0] & (1u << 4)) --left_y;
        if (pad.button[0] & (1u << 6)) ++left_y;
        if (left_y < 1) left_y = 1;
        if (left_y > HEIGHT - 2) left_y = HEIGHT - 2;

        if (right_y < ball_y) ++right_y;
        if (right_y > ball_y) --right_y;
        ball_x += vx;
        ball_y += vy;
        if (ball_y <= 1 || ball_y >= HEIGHT - 2) vy = -vy;
        if (ball_x == 3 && ball_y >= left_y - 1 && ball_y <= left_y + 1) vx = 1;
        if (ball_x == WIDTH - 4 && ball_y >= right_y - 1 && ball_y <= right_y + 1) vx = -1;
        if (ball_x <= 0) { ++right_score; ball_x = WIDTH / 2; ball_y = HEIGHT / 2; vx = 1; }
        if (ball_x >= WIDTH - 1) { ++left_score; ball_x = WIDTH / 2; ball_y = HEIGHT / 2; vx = -1; }

        /* Keep the RSX command path active; drawing setup is the next stage. */
        rsxFlushBuffer(context);
        sysUtilCheckCallback();
    }

    rsxFinish(context, 1);
    ioPadEnd();
    return (left_score + right_score) ? 0 : 0;
}
