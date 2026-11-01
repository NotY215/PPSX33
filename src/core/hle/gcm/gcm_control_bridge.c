#include <stdint.h>
/* Guest-visible control register helpers (put/get/ref offsets). */
uint32_t gcm_ctrl_put_off(void) { return 0; }
uint32_t gcm_ctrl_get_off(void) { return 4; }
uint32_t gcm_ctrl_ref_off(void) { return 8; }
