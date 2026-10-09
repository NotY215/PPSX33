// Phase 5 placeholder: RSX (GPU) translation layer.
// TODO: parse the RSX FIFO command buffer, translate NV47 state + vertex/fragment programs to
// HLSL/SPIR-V, expose backends: D3D10, D3D11, Vulkan. See ROADMAP.md "Phase 5".
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
enum Ps3GfxBackend { PS3_GFX_NONE = 0, PS3_GFX_D3D10 = 1, PS3_GFX_D3D11 = 2, PS3_GFX_VULKAN = 3 };
static int g_backend = PS3_GFX_NONE;
PS3RT_API void ps3rt_set_graphics_backend(int backend) { g_backend = backend; }
PS3RT_API int  ps3rt_get_graphics_backend(void) { return g_backend; }
