#include "spurs_stub.h"

static int s_job;

int ps3rt_spurs_submit_job(uint32_t entry, uint32_t arg0, uint32_t arg1) {
    (void)arg0;
    (void)arg1;
    if (!entry) return -1;
    return ++s_job;
}
