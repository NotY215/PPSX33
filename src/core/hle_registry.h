// GPT-audit HLE registry surface (header-only tracking helpers)
// Full per-NID implementations remain in hle_kernel_syscalls.inl / lv2_syscalls.inl.
#pragma once
#include <cstdint>

enum class HleImplState : uint8_t {
    Unresolved = 0,
    Stubbed = 1,
    Partial = 2,
    Implemented = 3
};

struct HleImportInfo {
    const char* library;
    uint32_t nid;
    HleImplState state;
    const char* note;
};

// Query helpers used by diagnostics; expand as imports are filled in.
inline const char* hle_state_name(HleImplState s) {
    switch (s) {
    case HleImplState::Implemented: return "implemented";
    case HleImplState::Partial: return "partial";
    case HleImplState::Stubbed: return "stubbed";
    default: return "unresolved";
    }
}
