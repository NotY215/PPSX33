# Phase XXIII — Execution reality (ACTIVE)

Addresses control-flow collapse (null PC), present path without guest shaders, and audio host keep-alive observed on GOW III logs. Full pipeline accuracy remains open work.

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **XXIII-A** | DONE | Null-PC recovery to last-good / first chunk | `runtime/ppu_runtime_run.h` |
| **XXIII-B** | DONE | Present color/tex every flip even when shaders=0 | `runtime/rsx_stub.cpp` |
| **XXIII-C** | DONE | Audio host open on tick | `runtime/audio_save.cpp` |
| **XXIII-D** | DONE | PPU opcode semantics markers (sync / unknown XO) | lifter + `ppu_runtime_*` |
| **XXIII-E** | DONE | RSX present path + more FIFO methods | `rsx_stub.cpp` |
| **XXIII-F** | DONE | SPU/MFC tag update, list-stall ack, atomic HLE | `spu_stub.cpp` |
| **XXIII-G** | DONE | LV2 wait/signal contracts (mutex/cond/sem/event/timer) | `lv2_syscalls.inl` |
| **XXIII-H** | DONE | PRX import NID recovery + module bind | `ps3rt_b.inl` |
| **XXIII-I** | PLANNED | cellFs / PSARC fidelity | `assets.cpp` |
| **XXIII-J** | PLANNED | SPURS completion | `spu_stub.cpp` |
| **XXIII-K** | PLANNED | Timing model | `ps3rt_b.inl` |
| **XXIII-L** | PLANNED | Coverage metrics (semantic vs decode) | docs |

## GOW III log notes

- `wild-pc 0x0` / `heartbeat pc=0x0` → XXIII-A
- `shaders=0 tex=0 methods=4` with synthetic clears → XXIII-B/E (present path only; not full 3D)
- `prx_import_stub` with nid==stub PC → XXIII-H NID recovery from GPRs / LR
- LV2 sc traces → XXIII-G contract logging

## Build package cleanup

See `src/core/project.cpp` `cleanup_after_build`: lean `output/`; remove `codebase/` + `input/` after successful build.
