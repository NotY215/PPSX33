#pragma once
#include <stdint.h>
#ifndef PPSX33_VK_API
#  if defined(_WIN32) && defined(PPSX33_API_BUILD)
#    define PPSX33_VK_API __declspec(dllexport)
#  elif defined(_WIN32)
#    define PPSX33_VK_API __declspec(dllimport)
#  else
#    define PPSX33_VK_API
#  endif
#endif
#ifdef __cplusplus
extern "C" {
#endif
PPSX33_VK_API int  rsx_vk_init(void* hwnd, uint32_t width, uint32_t height);
PPSX33_VK_API void rsx_vk_shutdown(void);
PPSX33_VK_API int  rsx_vk_ready(void);
PPSX33_VK_API int  rsx_vk_present_clear(float r, float g, float b, float a);
PPSX33_VK_API int  rsx_vk_present_guest_rgba(const void* pixels, uint32_t w, uint32_t h, uint32_t pitch_bytes);
PPSX33_VK_API void rsx_vk_poll_window(void);
#ifdef __cplusplus
}
#endif
