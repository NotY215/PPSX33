# Phase XXV — Truthful execution & first real pixels (ACTIVE)

**Last updated:** 2026-10-25

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **A–B** | DONE (untested) | Audio host; visible present | `audio_save.cpp`, `rsx_stub.cpp` |
| **C / E** | DONE (untested) | Lifter honesty; present order; solid triangle | `ppu_lifter.cpp`, `rsx_stub.cpp` |
| **D** | DONE (untested) | Full depth FIFO/surface/host draw (on E) | `rsx_stub.cpp`, `ps3rt_b.inl` |
| **F** | DONE (untested) | Deterministic faults / recovery visibility | `ppu_runtime_run.h` |
| **G–J** | PLANNED | PRX/LV2, SPU/MFC, services, E2E | |

### XXV-F full depth

| API | Role |
|-----|------|
| `validate_guest_pc` | translated / import_stub / guest_mem / invalid |
| `resolve_function_descriptor` | OPD entry + TOC → r2 |
| `resolve_guest_branch` | accept only classified targets (no silent retarget) |
| `capture_guest_fault` | PC/LR/CTR/TOC/SP/GPRs + stack walk; sets `halted` |
| `log_recovery_action` | every soft recovery logged |
| Storm limit | same-PC / total recoveries → fault + halt |
| Strict mode | env `PS3RT_STRICT_FAULT=1` — invalid PC faults instead of inventing return |

Default remains soft-continue for experiment; strict is opt-in. Recovery is never silent.

### Log signals (F)

- `[ps3] XXV-F recover …`
- `[ps3] XXV-F FAULT …` / `callstack[n]`
- `[ps3] XXV-F STRICT_FAULT=1` at run start when enabled

## Related

[ROADMAP.md](../ROADMAP.md) · [XXIV.md](XXIV.md)
