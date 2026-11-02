#pragma once
#ifndef PPSX33_RSX_API
#  if defined(_WIN32) && defined(PPSX33_RSX_BUILD)
#    define PPSX33_RSX_API __declspec(dllexport)
#  elif defined(_WIN32)
#    define PPSX33_RSX_API __declspec(dllimport)
#  else
#    define PPSX33_RSX_API
#  endif
#endif
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t color_offset;
    uint32_t pitch;
    uint32_t format;
    uint32_t zeta_offset;
    uint32_t width, height;
} rsx_dsp_surface;

typedef struct {
    uint32_t x, y, w, h;
} rsx_dsp_viewport;

typedef struct rsx_dispatch_sink {
    void* user;
    void (*on_surface)(void* user, const rsx_dsp_surface* s);
    void (*on_viewport)(void* user, const rsx_dsp_viewport* v);
    void (*on_draw)(void* user, uint32_t method, uint32_t arg);
    void (*on_flip)(void* user, uint32_t buffer_id, uint32_t color_offset, uint32_t pitch);
} rsx_dispatch_sink;

typedef struct {
    rsx_dsp_surface surface;
    rsx_dsp_viewport viewport;
    rsx_dsp_viewport scissor;
    uint32_t last_method;
    uint32_t draw_count;
    uint32_t flip_count;
    int has_surface;
    int has_viewport;
    rsx_dispatch_sink sink;
} rsx_dispatch;

PPSX33_RSX_API void rsx_dispatch_init(rsx_dispatch* d, const rsx_dispatch_sink* sink);
PPSX33_RSX_API void rsx_dispatch_reset(rsx_dispatch* d);
PPSX33_RSX_API void rsx_dispatch_method(rsx_dispatch* d, uint32_t method, uint32_t arg);
PPSX33_RSX_API void rsx_dispatch_flip(rsx_dispatch* d, uint32_t buffer_id);
PPSX33_RSX_API void rsx_dispatch_get_surface(const rsx_dispatch* d, rsx_dsp_surface* out);
PPSX33_RSX_API void rsx_dispatch_get_viewport(const rsx_dispatch* d, rsx_dsp_viewport* out);

/* Process-global dispatch used by host flip path (optional). */
PPSX33_RSX_API rsx_dispatch* rsx_dispatch_global(void);
PPSX33_RSX_API void rsx_dispatch_global_init(const rsx_dispatch_sink* sink);

#ifdef __cplusplus
}
#endif
