#include "rsx_vk_spirv_blit.h"
/* Full SPIR-V modules for textured blit land when guest RGBA pipeline needs GPU shader path.
 * Present currently uses clear + buffer-to-image copy; this module is the hook for shader blit. */
static int s_avail = 0;
int rsx_vk_spirv_blit_available(void) { return s_avail; }
int rsx_vk_spirv_blit_init(void*, void*) { s_avail = 0; return 0; }
void rsx_vk_spirv_blit_shutdown(void*) { s_avail = 0; }
int rsx_vk_spirv_blit_draw(void*) { return 0; }
