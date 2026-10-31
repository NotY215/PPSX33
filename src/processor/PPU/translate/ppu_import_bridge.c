#include "ppu_import_bridge.h"
#include "import_stub_map.h"

int ps3rt_is_import_pc(uint64_t pc) {
    return pc == 0x39800000ull || (pc & 0xFFFF0000ull) == 0x39800000ull;
}

uint32_t ps3rt_resolve_import_nid(uint64_t pc, uint64_t lr, uint32_t maybe_r11) {
    uint32_t n = ps3rt_import_lookup(lr);
    if (!n) n = ps3rt_import_lookup(pc);
    if (!n) n = maybe_r11;
    return n;
}
