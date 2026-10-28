#pragma once
// Formal result of decoding one PPU instruction (Phase 1 direction).
#include <cstdint>

namespace ppsx33 {
namespace ppu {

enum class SemanticKind : uint8_t {
    FullyImplemented = 0,
    UnsupportedForm = 1,
    ReservedOrInvalid = 2,
    RecognizedUnverified = 3, // emitted code exists; not differential-tested
};

struct SemanticResult {
    SemanticKind kind = SemanticKind::ReservedOrInvalid;
    uint32_t primary_opcode = 0;
    uint32_t form = 0;
    const char* name = nullptr;
};

} // namespace ppu
} // namespace ppsx33
