#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef PPSX33_GL_API
#  if defined(_WIN32) && defined(PS3RT_BUILD_DLL)
#    define PPSX33_GL_API __declspec(dllexport)
#  else
#    define PPSX33_GL_API
#  endif
#endif

PPSX33_GL_API int  rsx_gl_init(void* hwnd, uint32_t width, uint32_t height);
PPSX33_GL_API void rsx_gl_shutdown(void);
PPSX33_GL_API int  rsx_gl_ready(void);
PPSX33_GL_API void rsx_gl_poll_window(void);
PPSX33_GL_API int  rsx_gl_present_clear(float r, float g, float b, float a);
PPSX33_GL_API int  rsx_gl_present_guest_rgba(const void* pixels, uint32_t w, uint32_t h, uint32_t pitch_bytes);
PPSX33_GL_API void rsx_gl_resize(uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif
