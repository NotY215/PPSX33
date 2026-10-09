/*
 * PPSX33 Pong sample: deterministic text-mode simulation for PPC64 testing.
 * The PS3 ELF/OPD packaging and console I/O layer depend on the chosen SDK.
 */
#include <stdint.h>
#include <stdio.h>

enum { WIDTH = 40, HEIGHT = 14 };
static void draw(int bx, int by, int lp, int rp, unsigned score_l, unsigned score_r) {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            char c = ' ';
            if (x == 0 || x == WIDTH - 1) c = '|';
            if (x == 2 && y >= lp && y < lp + 3) c = '#';
            if (x == WIDTH - 3 && y >= rp && y < rp + 3) c = '#';
            if (x == WIDTH / 2) c = ':';
            if (x == bx && y == by) c = 'O';
            putchar(c);
        }
        putchar('\n');
    }
    printf("PPSX33 PONG  %u : %u\n", score_l, score_r);
}
int main(void) {
    int bx = WIDTH / 2, by = HEIGHT / 2, vx = 1, vy = 1;
    int lp = HEIGHT / 2 - 1, rp = HEIGHT / 2 - 1;
    unsigned sl = 0, sr = 0;
    for (unsigned frame = 0; frame < 240; ++frame) {
        /* Simple deterministic paddle AI keeps the sample reproducible. */
        if (rp + 1 < by) ++rp; else if (rp + 1 > by) --rp;
        if (lp + 1 < by) ++lp; else if (lp + 1 > by) --lp;
        bx += vx; by += vy;
        if (by <= 0 || by >= HEIGHT - 1) vy = -vy;
        if (bx == 3 && by >= lp && by < lp + 3) vx = 1;
        if (bx == WIDTH - 4 && by >= rp && by < rp + 3) vx = -1;
        if (bx <= 0) { ++sr; bx = WIDTH / 2; by = HEIGHT / 2; vx = 1; }
        if (bx >= WIDTH - 1) { ++sl; bx = WIDTH / 2; by = HEIGHT / 2; vx = -1; }
        if ((frame % 24) == 0) draw(bx, by, lp, rp, sl, sr);
    }
    return 0;
}
