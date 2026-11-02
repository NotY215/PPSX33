#include "rsx_dispatch_sink.h"
#include "rsx_method_table.h"
#include <string.h>

static rsx_dispatch g_dispatch;

void rsx_dispatch_init(rsx_dispatch* d, const rsx_dispatch_sink* sink) {
    if (!d) return;
    memset(d, 0, sizeof(*d));
    d->surface.width = 720;
    d->surface.height = 576;
    d->surface.pitch = 720 * 4;
    d->viewport.w = 720;
    d->viewport.h = 576;
    d->scissor = d->viewport;
    if (sink) d->sink = *sink;
}

void rsx_dispatch_reset(rsx_dispatch* d) {
    if (!d) return;
    rsx_dispatch_sink sink = d->sink;
    rsx_dispatch_init(d, &sink);
}

void rsx_dispatch_method(rsx_dispatch* d, uint32_t method, uint32_t arg) {
    if (!d) return;
    d->last_method = method;
    rsx_method_kind k = rsx_classify_method(method);
    switch (k) {
    case RSX_M_SURFACE_COLOR_OFFSET:
        d->surface.color_offset = arg;
        d->has_surface = 1;
        if (d->sink.on_surface) d->sink.on_surface(d->sink.user, &d->surface);
        break;
    case RSX_M_SURFACE_PITCH:
        if (arg) d->surface.pitch = arg;
        d->has_surface = 1;
        break;
    case RSX_M_SURFACE_FORMAT:
        d->surface.format = arg;
        break;
    case RSX_M_SURFACE_ZETA_OFFSET:
        d->surface.zeta_offset = arg;
        break;
    case RSX_M_VIEWPORT:
        d->viewport.x = arg & 0xffffu;
        d->viewport.w = (arg >> 16) & 0xffffu;
        if (!d->viewport.w) d->viewport.w = 720;
        d->has_viewport = 1;
        if (d->sink.on_viewport) d->sink.on_viewport(d->sink.user, &d->viewport);
        break;
    case RSX_M_SCISSOR:
        d->scissor.x = arg & 0xffffu;
        d->scissor.w = (arg >> 16) & 0xffffu;
        break;
    case RSX_M_DRAW:
    case RSX_M_BEGIN_END:
        d->draw_count++;
        if (d->sink.on_draw) d->sink.on_draw(d->sink.user, method, arg);
        break;
    case RSX_M_CLEAR:
        d->draw_count++;
        if (d->sink.on_draw) d->sink.on_draw(d->sink.user, method, arg);
        break;
    default:
        break;
    }
}

void rsx_dispatch_flip(rsx_dispatch* d, uint32_t buffer_id) {
    if (!d) return;
    d->flip_count++;
    if (d->sink.on_flip) {
        d->sink.on_flip(d->sink.user, buffer_id, d->surface.color_offset, d->surface.pitch);
    }
}

void rsx_dispatch_get_surface(const rsx_dispatch* d, rsx_dsp_surface* out) {
    if (!d || !out) return;
    *out = d->surface;
}

void rsx_dispatch_get_viewport(const rsx_dispatch* d, rsx_dsp_viewport* out) {
    if (!d || !out) return;
    *out = d->viewport;
}

rsx_dispatch* rsx_dispatch_global(void) { return &g_dispatch; }

void rsx_dispatch_global_init(const rsx_dispatch_sink* sink) {
    rsx_dispatch_init(&g_dispatch, sink);
}
