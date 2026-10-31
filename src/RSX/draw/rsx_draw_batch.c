#include "rsx_draw_batch.h"

static rsx_draw_frame g_cur;
static int g_dirty;

void rsx_draw_batch_reset(void) {
    g_cur.color_offset = 0;
    g_cur.depth_offset = 0;
    g_cur.width = 720;
    g_cur.height = 576;
    g_cur.pitch = 720 * 4;
    g_cur.tex_offset = 0;
    g_cur.draw_count = 0;
    g_cur.clear_mask = 0;
    g_cur.frame_seq = 0;
    g_dirty = 0;
}

void rsx_draw_batch_set_color(uint32_t offset, uint32_t pitch, uint32_t w, uint32_t h) {
    g_cur.color_offset = offset;
    g_cur.pitch = pitch ? pitch : (720u * 4u);
    g_cur.width = 720;
    g_cur.height = 576;
    (void)w; (void)h;
    g_dirty = 1;
}

void rsx_draw_batch_set_tex(uint32_t offset) {
    g_cur.tex_offset = offset;
    g_dirty = 1;
}

void rsx_draw_batch_note_draw(void) {
    g_cur.draw_count++;
    g_dirty = 1;
}

void rsx_draw_batch_note_clear(uint32_t mask) {
    g_cur.clear_mask |= mask;
    g_dirty = 1;
}

const rsx_draw_frame* rsx_draw_batch_current(void) { return &g_cur; }

int rsx_draw_batch_take_frame(rsx_draw_frame* out) {
    if (!out || !g_dirty) return 0;
    g_cur.frame_seq++;
    *out = g_cur;
    g_cur.draw_count = 0;
    g_cur.clear_mask = 0;
    g_dirty = 0;
    return 1;
}
