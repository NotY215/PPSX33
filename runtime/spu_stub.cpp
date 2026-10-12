// Phase U: lightweight SPU step helper used by ps3rt SPU HLE
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstring>

// Optional cooperative SPU "tick": if a context is running, advance PC and
// post a completion word to mailbox_out so PPU waiters can proceed.
extern "C" PS3RT_API void ps3rt_spu_tick_all(void) {
    // Implemented in ps3rt.cpp when linked; weak stub here for link safety.
}
