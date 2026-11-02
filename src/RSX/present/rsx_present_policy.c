#include "rsx_present_policy.h"

rsx_present_action rsx_present_decide(
    int has_guest_color,
    int methods_this_frame,
    int draws_this_frame,
    uint32_t flip_count) {
    (void)flip_count;
    if (has_guest_color && (draws_this_frame > 0 || methods_this_frame > 0))
        return RSX_PRESENT_GUEST;
    if (has_guest_color)
        return RSX_PRESENT_GUEST;
    /* Idle title stream: still clear so window stays alive. */
    return RSX_PRESENT_CLEAR;
}

int rsx_present_clear_interval_ms(void) { return 16; }
