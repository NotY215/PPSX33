// SPU support is implemented in ps3rt.cpp (create/load/run/mbox/MFC).
// This file remains so existing build lists that compile spu_stub.cpp keep linking.
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"

// ps3rt_spu_supported is defined in ps3rt.cpp; no extra symbols here.
