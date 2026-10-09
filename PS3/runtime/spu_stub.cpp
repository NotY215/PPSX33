// Phase 4 placeholder: SPU (Synergistic Processing Unit) support.
// TODO: lift SPU ELF images (embedded in the game) the same way as the PPU, one host thread per SPU
// thread group, DMA via the MFC, mailboxes/signals. See ROADMAP.md "Phase 4".
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
PS3RT_API int ps3rt_spu_supported(void) { return 0; }
