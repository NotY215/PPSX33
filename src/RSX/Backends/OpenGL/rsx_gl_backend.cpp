#include "rsx_gl_backend.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>

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
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#endif

typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);

typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, ptrdiff_t, const void*, GLenum);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const char* const*, const GLint*);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const char*);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void (APIENTRY *PFNGLTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);

static HWND   s_hwnd = nullptr;
static HDC    s_dc = nullptr;
static HGLRC  s_rc = nullptr;
static int    s_ready = 0;
static int    s_core = 0;
static uint32_t s_w = 720, s_h = 560;
static GLuint s_tex = 0;
static GLuint s_prog = 0;
static GLuint s_vao = 0;
static GLuint s_vbo = 0;
static int    s_frame = 0;

extern "C" void ps3rt_log(const char* fmt, ...);
extern "C" void ps3rt_request_exit(void);

static void* gl_proc(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (!p || p == (void*)0x1 || p == (void*)0x2 || p == (void*)0x3 || p == (void*)-1) {
        HMODULE m = GetModuleHandleA("opengl32.dll");
        if (m) p = (void*)GetProcAddress(m, name);
    }
    return p;
}

static int gl_build_blit_program(void) {
    auto CreateShader = (PFNGLCREATESHADERPROC)gl_proc("glCreateShader");
    auto ShaderSource = (PFNGLSHADERSOURCEPROC)gl_proc("glShaderSource");
    auto CompileShader = (PFNGLCOMPILESHADERPROC)gl_proc("glCompileShader");
    auto GetShaderiv = (PFNGLGETSHADERIVPROC)gl_proc("glGetShaderiv");
    auto CreateProgram = (PFNGLCREATEPROGRAMPROC)gl_proc("glCreateProgram");
    auto AttachShader = (PFNGLATTACHSHADERPROC)gl_proc("glAttachShader");
    auto LinkProgram = (PFNGLLINKPROGRAMPROC)gl_proc("glLinkProgram");
    auto GetProgramiv = (PFNGLGETPROGRAMIVPROC)gl_proc("glGetProgramiv");
    auto DeleteShader = (PFNGLDELETESHADERPROC)gl_proc("glDeleteShader");
    auto GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)gl_proc("glGenVertexArrays");
    auto BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)gl_proc("glBindVertexArray");
    auto GenBuffers = (PFNGLGENBUFFERSPROC)gl_proc("glGenBuffers");
    auto BindBuffer = (PFNGLBINDBUFFERPROC)gl_proc("glBindBuffer");
    auto BufferData = (PFNGLBUFFERDATAPROC)gl_proc("glBufferData");
    auto EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)gl_proc("glEnableVertexAttribArray");
    auto VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)gl_proc("glVertexAttribPointer");
    if (!CreateShader || !CreateProgram || !GenVertexArrays) return 0;

    static const char* vs_src =
        "#version 150\n"
        "in vec2 a_pos;\n"
        "in vec2 a_uv;\n"
        "out vec2 v_uv;\n"
        "void main(){ v_uv=a_uv; gl_Position=vec4(a_pos,0.0,1.0); }\n";
    static const char* fs_src =
        "#version 150\n"
        "uniform sampler2D u_tex;\n"
        "in vec2 v_uv;\n"
        "out vec4 o_col;\n"
        "void main(){ o_col = texture(u_tex, v_uv); }\n";

    GLuint vs = CreateShader(GL_VERTEX_SHADER);
    GLuint fs = CreateShader(GL_FRAGMENT_SHADER);
    ShaderSource(vs, 1, &vs_src, nullptr);
    ShaderSource(fs, 1, &fs_src, nullptr);
    CompileShader(vs);
    CompileShader(fs);
    GLint ok = 0;
    GetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) return 0;
    GetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) return 0;
    s_prog = CreateProgram();
    AttachShader(s_prog, vs);
    AttachShader(s_prog, fs);
    LinkProgram(s_prog);
    GetProgramiv(s_prog, GL_LINK_STATUS, &ok);
    DeleteShader(vs);
    DeleteShader(fs);
    if (!ok) { s_prog = 0; return 0; }

    /* fullscreen triangle strip: pos.xy uv.xy */
    const float verts[] = {
        -1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, 1.f, 1.f,
        -1.f,  1.f, 0.f, 0.f,
         1.f,  1.f, 1.f, 0.f,
    };
    GenVertexArrays(1, &s_vao);
    GenBuffers(1, &s_vbo);
    BindVertexArray(s_vao);
    BindBuffer(GL_ARRAY_BUFFER, s_vbo);
    BufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    EnableVertexAttribArray(0);
    VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (const void*)0);
    EnableVertexAttribArray(1);
    VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (const void*)8);
    BindVertexArray(0);
    return 1;
}

static void gl_draw_textured_fullscreen(void) {
    auto UseProgram = (PFNGLUSEPROGRAMPROC)gl_proc("glUseProgram");
    auto BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)gl_proc("glBindVertexArray");
    auto GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)gl_proc("glGetUniformLocation");
    auto Uniform1i = (PFNGLUNIFORM1IPROC)gl_proc("glUniform1i");
    auto ActiveTexture = (PFNGLACTIVETEXTUREPROC)gl_proc("glActiveTexture");
    if (s_prog && UseProgram && BindVertexArray) {
        UseProgram(s_prog);
        if (ActiveTexture) ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_tex);
        if (GetUniformLocation && Uniform1i) {
            GLint loc = GetUniformLocation(s_prog, "u_tex");
            if (loc >= 0) Uniform1i(loc, 0);
        }
        BindVertexArray(s_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        BindVertexArray(0);
        UseProgram(0);
        return;
    }
    /* Compatibility fallback */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_tex);
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
}

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
    HGLRC tmp = wglCreateContext(s_dc);
    if (!tmp || !wglMakeCurrent(s_dc, tmp)) {
        if (tmp) wglDeleteContext(tmp);
        ReleaseDC(s_hwnd, s_dc); s_dc = nullptr;
        return 0;
    }

    auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB) {
        /* Prefer 4.5 compatibility so present works even without full shader path */
        const int attribs_compat[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
            0
        };
        const int attribs_core[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        HGLRC modern = wglCreateContextAttribsARB(s_dc, nullptr, attribs_compat);
        if (!modern) {
            modern = wglCreateContextAttribsARB(s_dc, nullptr, attribs_core);
            if (modern) s_core = 1;
        }
        if (modern) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(tmp);
            s_rc = modern;
            wglMakeCurrent(s_dc, s_rc);
            ps3rt_log("[rsx] OpenGL 4.5 %s context\n", s_core ? "core" : "compatibility");
        } else {
            s_rc = tmp;
            ps3rt_log("[rsx] OpenGL legacy context (4.5 request failed)\n");
        }
    } else {
        s_rc = tmp;
        ps3rt_log("[rsx] OpenGL legacy context\n");
    }

    glGenTextures(1, &s_tex);
    glBindTexture(GL_TEXTURE_2D, s_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)s_w, (GLsizei)s_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (!gl_build_blit_program()) {
        ps3rt_log("[rsx] OpenGL: shader blit unavailable — using fixed pipeline if allowed\n");
    } else {
        ps3rt_log("[rsx] OpenGL: shader fullscreen blit ready\n");
    }

    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) wglSwapIntervalEXT(1);

    s_ready = 1;
    ps3rt_log("[rsx] OpenGL 4.5: context ready %ux%u\n", (unsigned)s_w, (unsigned)s_h);
    return 1;
}

void rsx_gl_shutdown(void) {
    if (s_rc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(s_rc);
        s_rc = nullptr;
    }
    if (s_dc && s_hwnd) { ReleaseDC(s_hwnd, s_dc); s_dc = nullptr; }
    s_hwnd = nullptr;
    s_ready = 0;
    s_tex = s_prog = s_vao = s_vbo = 0;
}

int rsx_gl_ready(void) { return s_ready; }

void rsx_gl_poll_window(void) {
    if (!s_hwnd) return;
    MSG msg;
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
    if (!wglMakeCurrent(s_dc, s_rc)) return 0;
    glViewport(0, 0, (GLsizei)s_w, (GLsizei)s_h);
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(s_dc);
    ++s_frame;
    if (s_frame <= 3 || (s_frame & 0x3F) == 0)
        ps3rt_log("[rsx] OpenGL present clear frame=%d\n", s_frame);
    return 1;
}

int rsx_gl_present_guest_rgba(const void* pixels, uint32_t w, uint32_t h, uint32_t pitch_bytes) {
    if (!s_ready || !pixels || !w || !h) return 0;
    if (!wglMakeCurrent(s_dc, s_rc)) return 0;
    glViewport(0, 0, (GLsizei)s_w, (GLsizei)s_h);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, s_tex);

    const uint8_t* src = (const uint8_t*)pixels;
    uint32_t row_bytes = w * 4u;
    if (pitch_bytes == 0) pitch_bytes = row_bytes;

    if (w != s_w || h != s_h) {
        s_w = w; s_h = h;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    if (pitch_bytes == row_bytes) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)w, (GLsizei)h, GL_RGBA, GL_UNSIGNED_BYTE, src);
    } else {
        for (uint32_t y = 0; y < h; ++y)
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)y, (GLsizei)w, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                            src + (size_t)y * pitch_bytes);
    }

    glClearColor(0.02f, 0.02f, 0.05f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    gl_draw_textured_fullscreen();
    SwapBuffers(s_dc);
    ++s_frame;
    if (s_frame <= 8 || (s_frame & 0x1F) == 1)
        ps3rt_log("[rsx] OpenGL guest present frame=%d %ux%u pitch=%u\n", s_frame, (unsigned)w, (unsigned)h, (unsigned)pitch_bytes);
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
