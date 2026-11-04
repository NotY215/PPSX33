#include "rsx_gl_backend.h"
#include <cstring>
#include <cstdlib>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

static HWND   s_hwnd = nullptr;
static HDC    s_dc = nullptr;
static HGLRC  s_rc = nullptr;
static int    s_ready = 0;
static uint32_t s_w = 720, s_h = 560;
static GLuint s_tex = 0;

static void gl_log(const char* m) {
    OutputDebugStringA(m);
    OutputDebugStringA("\n");
}

extern "C" void ps3rt_log(const char* fmt, ...);
extern "C" void ps3rt_request_exit(void);

int rsx_gl_init(void* hwnd, uint32_t width, uint32_t height) {
    if (s_ready) return 1;
    s_hwnd = (HWND)hwnd;
    if (!s_hwnd) return 0;
    if (width) s_w = width;
    if (height) s_h = height;

    s_dc = GetDC(s_hwnd);
    if (!s_dc) return 0;

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    int pf = ChoosePixelFormat(s_dc, &pfd);
    if (!pf || !SetPixelFormat(s_dc, pf, &pfd)) {
        ReleaseDC(s_hwnd, s_dc); s_dc = nullptr;
        return 0;
    }
    s_rc = wglCreateContext(s_dc);
    if (!s_rc || !wglMakeCurrent(s_dc, s_rc)) {
        if (s_rc) wglDeleteContext(s_rc);
        s_rc = nullptr;
        ReleaseDC(s_hwnd, s_dc); s_dc = nullptr;
        return 0;
    }

    glGenTextures(1, &s_tex);
    glBindTexture(GL_TEXTURE_2D, s_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)s_w, (GLsizei)s_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    
    /* Prefer OpenGL 4.5 core when available */
    typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
    auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB) {
        const int attribs[] = {
            0x2091 /*WGL_CONTEXT_MAJOR_VERSION_ARB*/, 4,
            0x2092 /*WGL_CONTEXT_MINOR_VERSION_ARB*/, 5,
            0x9126 /*WGL_CONTEXT_PROFILE_MASK_ARB*/, 1 /*CORE*/,
            0
        };
        HGLRC modern = wglCreateContextAttribsARB(s_dc, nullptr, attribs);
        if (modern) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(s_rc);
            s_rc = modern;
            wglMakeCurrent(s_dc, s_rc);
            ps3rt_log("[rsx] OpenGL 4.5 core context\n");
        } else {
            ps3rt_log("[rsx] OpenGL 4.5 core unavailable — using compatibility context\n");
        }
    }

    glViewport(0, 0, (GLsizei)s_w, (GLsizei)s_h);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* VSync when extension present */
    typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) wglSwapIntervalEXT(1);
    s_ready = 1;
    ps3rt_log("[rsx] OpenGL 4.5: context ready %ux%u\n", s_w, s_h);
    ps3rt_log("[rsx] OpenGL 4.5 host present ready %ux%u images=1\n", s_w, s_h);
    return 1;
}

void rsx_gl_shutdown(void) {
    if (s_rc) {
        wglMakeCurrent(s_dc, s_rc);
        if (s_tex) { glDeleteTextures(1, &s_tex); s_tex = 0; }
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(s_rc);
        s_rc = nullptr;
    }
    if (s_dc && s_hwnd) { ReleaseDC(s_hwnd, s_dc); s_dc = nullptr; }
    s_hwnd = nullptr;
    s_ready = 0;
}

int rsx_gl_ready(void) { return s_ready; }

void rsx_gl_poll_window(void) {
    if (!s_hwnd) return;
    if (!IsWindow((HWND)s_hwnd)) {
        s_hwnd = nullptr;
        s_ready = 0;
        ps3rt_request_exit();
        return;
    }
    MSG msg;
    /* Thread-wide queue so WM_QUIT is not missed */
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            s_ready = 0;
            s_hwnd = nullptr;
            ps3rt_request_exit();
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void rsx_gl_resize(uint32_t width, uint32_t height) {
    if (!s_ready) return;
    if (width) s_w = width;
    if (height) s_h = height;
    wglMakeCurrent(s_dc, s_rc);
    glViewport(0, 0, (GLsizei)s_w, (GLsizei)s_h);
}

int rsx_gl_present_clear(float r, float g, float b, float a) {
    if (!s_ready) return 0;
    wglMakeCurrent(s_dc, s_rc);
    glViewport(0, 0, (GLsizei)s_w, (GLsizei)s_h);
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(s_dc);
    return 1;
}

int rsx_gl_present_guest_rgba(const void* pixels, uint32_t w, uint32_t h, uint32_t pitch_bytes) {
    if (!s_ready || !pixels || !w || !h) return 0;
    wglMakeCurrent(s_dc, s_rc);
    glViewport(0, 0, (GLsizei)s_w, (GLsizei)s_h);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_tex);

    /* Upload row-by-row if pitch != w*4 */
    const uint8_t* src = (const uint8_t*)pixels;
    uint32_t row_bytes = w * 4u;
    if (pitch_bytes == 0) pitch_bytes = row_bytes;
    if (pitch_bytes == row_bytes) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, src);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        for (uint32_t y = 0; y < h; ++y)
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)y, (GLsizei)w, 1, GL_RGBA, GL_UNSIGNED_BYTE, src + (size_t)y * pitch_bytes);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(0, 0);
    glTexCoord2f(1, 1); glVertex2f(1, 0);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(0, 1);
    glEnd();
    SwapBuffers(s_dc);
    return 1;
}

#else
int rsx_gl_init(void*, uint32_t, uint32_t) { return 0; }
void rsx_gl_shutdown(void) {}
int rsx_gl_ready(void) { return 0; }
void rsx_gl_poll_window(void) {}
int rsx_gl_present_clear(float, float, float, float) { return 0; }
int rsx_gl_present_guest_rgba(const void*, uint32_t, uint32_t, uint32_t) { return 0; }
void rsx_gl_resize(uint32_t, uint32_t) {}
#endif
