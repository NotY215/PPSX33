#pragma once
#include <stdint.h>
/* Host fullscreen blit program state — SPIR-V modules optional; clear path always works. */
#ifdef __cplusplus
extern "C" {
#endif
int  rsx_vk_spirv_blit_available(void);
int  rsx_vk_spirv_blit_init(void* device /*VkDevice*/, void* render_pass);
void rsx_vk_spirv_blit_shutdown(void* device);
int  rsx_vk_spirv_blit_draw(void* cmd /*VkCommandBuffer*/);
#ifdef __cplusplus
}
#endif
