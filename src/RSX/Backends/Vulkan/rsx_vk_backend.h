// WORK: Vulkan 1.2 host present — instance, device, swapchain, clear+guest present.
#pragma once
#include <cstdint>

int  rsx_vk_init(void* hwnd, uint32_t width, uint32_t height);
void rsx_vk_shutdown(void);
int  rsx_vk_ready(void);
int  rsx_vk_present_clear(float r, float g, float b, float a);
int  rsx_vk_present_guest_rgba(const void* pixels, uint32_t w, uint32_t h, uint32_t pitch_bytes);
void rsx_vk_poll_window(void);
