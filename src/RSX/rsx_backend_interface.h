#pragma once
// WORK: Host backend strategy — Vulkan default, OpenGL secondary.
namespace ppsx33 { namespace rsx {
struct IBackend {
    virtual ~IBackend() = default;
    virtual bool init(void* hwnd, unsigned w, unsigned h) = 0;
    virtual void shutdown() = 0;
    virtual void present_clear(float r, float g, float b, float a) = 0;
    virtual bool present_guest_rgba(const void* pixels, unsigned w, unsigned h, unsigned pitch) = 0;
};
}}
