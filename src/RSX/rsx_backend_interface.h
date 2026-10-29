#pragma once
// WORK: Strategy interface for DX11/GL/Vulkan backends.
namespace ppsx33::rsx {
struct IBackend { virtual ~IBackend()=default; virtual bool init()=0; virtual void shutdown()=0; virtual void present()=0; };
}
