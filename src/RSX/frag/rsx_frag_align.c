#include "rsx_frag_align.h"
#include "rsx_frag_flip.h"
#include "audio_frag_service.h"

void rsx_frag_align_init(rsx_frag_align_ctx* ctx) {
    if (!ctx) return;
    ctx->mem = 0;
    ctx->mem_size = 0;
    ctx->ctrl_ea = 0;
    ctx->labels_ea = 0;
    ctx->fifo_base = 0;
    ctx->fifo_size = 0x100000u;
    ctx->frame_id = 0;
    rsx_frag_surface_reset(&ctx->surface);
    rsx_frag_fifo_stats_clear(&ctx->fifo);
    ctx->present.color_offset = 0;
    ctx->present.pitch = 720u * 4u;
    ctx->present.width = 720;
    ctx->present.height = 576;
    ctx->present.flags = 0;
}

void rsx_frag_align_tick(rsx_frag_align_ctx* ctx) {
    if (!ctx || !ctx->mem) return;
    ctx->frame_id++;

    /* FIFO: nudge control + walk slice */
    if (ctx->ctrl_ea) {
        rsx_frag_fifo_nudge_control(ctx->mem, ctx->mem_size, ctx->ctrl_ea, ctx->fifo_size);
        /* read put/get */
        if (ctx->ctrl_ea + 12 <= ctx->mem_size) {
            const uint8_t* p = ctx->mem + ctx->ctrl_ea;
            uint32_t put = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
            uint32_t get = ((uint32_t)p[8] << 24) | ((uint32_t)p[9] << 16) | ((uint32_t)p[10] << 8) | p[11];
            if (ctx->fifo_base)
                rsx_frag_fifo_walk(ctx->mem, ctx->mem_size, ctx->fifo_base, ctx->fifo_size, put, get, &ctx->fifo);
        }
    }

    /* Surface: if still empty, synthesize into guest RAM */
    if (!rsx_frag_surface_valid(&ctx->surface, ctx->mem_size)) {
        uint32_t off = 0x01000000u;
        uint32_t w = ctx->surface.width ? ctx->surface.width : 720u;
        uint32_t h = ctx->surface.height ? ctx->surface.height : 576u;
        rsx_frag_present_fill_pattern(ctx->mem, ctx->mem_size, off, w, h, ctx->frame_id);
        rsx_frag_surface_set_color(&ctx->surface, off, w * 4u, w, h);
    }

    rsx_frag_present_from_surface(&ctx->present, &ctx->surface);
    rsx_frag_flip_complete_label(ctx->mem, ctx->mem_size, ctx->labels_ea, ctx->frame_id);
    audio_frag_service_tick();
}

int rsx_frag_align_want_guest_present(const rsx_frag_align_ctx* ctx) {
    return ctx && ctx->present.flags == 2 && ctx->present.color_offset != 0;
}
