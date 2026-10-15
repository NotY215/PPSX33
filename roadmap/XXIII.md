# Phase XXIII — Execution reality (ACTIVE)

Addresses control-flow collapse (null PC), present path without guest shaders, and audio host keep-alive observed on GOW III logs. Full pipeline accuracy remains open work.

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **XXIII-A** | DONE | Null-PC recovery to last-good / first chunk | `runtime/ppu_runtime_run.h` |
| **XXIII-B** | DONE | Present color/tex every flip even when shaders=0 | `runtime/rsx_stub.cpp` |
| **XXIII-C** | DONE | Audio host open on tick | `runtime/audio_save.cpp` |
| **XXIII-D** | PLANNED | PPU opcode semantics audit | lifter + `ppu_runtime_*` |
| **XXIII-E** | PLANNED | Real RSX vertex/texture/shader path | `rsx_stub.cpp` |
| **XXIII-F** | PLANNED | SPU/MFC accuracy | `spu_stub.cpp` |
| **XXIII-G** | PLANNED | LV2 wait/signal contracts | `lv2_syscalls.inl` |
| **XXIII-H** | PLANNED | PRX reloc + import binding | `ps3rt_b.inl` |
| **XXIII-I** | PLANNED | cellFs / PSARC fidelity | `assets.cpp` |
| **XXIII-J** | PLANNED | SPURS completion | `spu_stub.cpp` |
| **XXIII-K** | PLANNED | Timing model | `ps3rt_b.inl` |
| **XXIII-L** | PLANNED | Coverage metrics (semantic vs decode) | docs |

## GOW III log notes

- `wild-pc 0x0` / `heartbeat pc=0x0` → XXIII-A
- `shaders=0 tex=0 methods=4` with synthetic clears → XXIII-B (present path only; not full 3D)
- Import-stub recovery remains expected until XXIII-H

## Build package cleanup

See `src/core/project.cpp` `cleanup_after_build`: lean `output/`; pruned `codebase/`; `input/` EBOOT only.
