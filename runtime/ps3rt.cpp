// ps3rt - runtime library (ps3rt.dll)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "nid_table.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>

// Minimal stub body so the build succeeds while the full interpreter is linked from the restored source.
// Full implementation follows after this marker.
// === FULL PS3RT BEGIN ===
