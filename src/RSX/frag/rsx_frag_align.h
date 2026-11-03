#pragma once
#include <stdint.h>
#include "rsx_frag_surface.h"
#include "rsx_frag_fifo.h"
#include "rsx_frag_present.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Orchestrates surface + fifo + present + flip label + audio service for one host tick. */
typedef struct rsx_frag_align_ctx {
    uint8_t* mem;
    uint64_t mem_size;
    uint64_t ctrl_ea;
    uint64_t labels_ea;
    uint64_t fifo_base;
    uint32_t fifo_size;
    uint32_t frame_id;
    rsx_frag_surface surface;
    rsx_frag_fifo_stats fifo;
    rsx_frag_present_req present;
} rsx_frag_align_ctx;

void rsx_frag_align_init(rsx_frag_align_ctx* ctx);
void rsx_frag_align_tick(rsx_frag_align_ctx* ctx);
/* Returns 1 if present request has guest color. */
int  rsx_frag_align_want_guest_present(const rsx_frag_align_ctx* ctx);

#ifdef __cplusplus
}
#endif
