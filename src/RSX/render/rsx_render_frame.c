#include "rsx_render_frame.h"
#include "rsx_present_queue.h"
#include "rsx_surface_fmt.h"

int rsx_render_frame_enqueue_present(void) {
    rsx_draw_frame fr;
    if (!rsx_draw_batch_take_frame(&fr)) return 0;
    rsx_present_req req;
    req.color_offset = fr.color_offset;
    req.width = 720;
    req.height = 576;
    req.pitch = fr.pitch ? fr.pitch : rsx_surface_fmt_pitch_576p(RSX_FMT_A8R8G8B8);
    req.flags = fr.color_offset ? 2u : 1u;
    req.frame_id = fr.frame_seq;
    return rsx_present_queue_push(&req);
}

int rsx_render_frame_on_flip(uint32_t color_offset, uint32_t pitch) {
    if (color_offset)
        rsx_draw_batch_set_color(color_offset, pitch, 720, 576);
    rsx_draw_batch_note_draw();
    return rsx_render_frame_enqueue_present();
}
