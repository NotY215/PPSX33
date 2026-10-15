# Phase XXII — Pipeline correctness (FINISHED)

**Track:** PHASE 1 after XXI (lettered A–Z)  
**Goal:** Reduce HLE lies and hard-stops called out in pipeline review (RSX, PPU/SPU, LV2, modules/FS, timing).  
**Not a claim:** commercial playability or full hardware accuracy.

---

## Subphase index

| ID | Name | Status | Primary files | Work |
|----|------|--------|---------------|------|
| **XXII-A** | RSX present / color-buffer | DONE | `runtime/rsx_stub.cpp` | Prefer guest color/tex on present; host upload/blit path |
| **XXII-B** | PPU soft-recovery | DONE | `runtime/ps3rt_b.inl`, `ppu_runtime_*.h` | Unimplemented ops soft-continue; shared lifter helpers |
| **XXII-C** | SPU / MFC fidelity | DONE | `runtime/spu_stub.cpp` | Tag/stall/mach-status surface |
| **XXII-D** | LV2 / HLE visibility | DONE | `runtime/ps3rt_b.inl`, `lv2_syscalls.inl` | Unknown NID / stub first-hit logging |
| **XXII-E** | Modules / PRX / FS | DONE | `runtime/ps3rt_b.inl`, `runtime/assets.cpp` | Ordered PRX registry + ELF/raw load; SELF skip |
| **XXII-F** | Timing / audio surface | DONE | `runtime/ps3rt_b.inl`, `runtime/audio_save.cpp` | Frame pacing helper; audio ring surface |
| **XXII-G** | RSX draw classification | DONE | `runtime/rsx_stub.cpp` | Textured vs none draw logs |
| **XXII-H** | PPU recovery surface | DONE | `ppu_runtime_run.h` | Soft recovery depth |
| **XXII-I** | SPU channel path | DONE | `runtime/spu_stub.cpp` | Channel continuity; case-fix for tag stat |
| **XXII-J** | LV2 contracts note | DONE | `lv2_syscalls.inl` | Upstream log / contract notes |
| **XXII-K** | Save / pad surface | DONE | `audio_save.cpp`, `ps3rt.cpp` | Lifecycle / pad tags |
| **XXII-L** | OpenGL probe prep | DONE | `rsx_stub.cpp` | D3D11 preference log; GL fallback not linked |

## Package cleanup

`cleanup_after_build` in `src/core/project.cpp`: lean `output/`; prune `codebase/`; `input/` EBOOT only.

## Next

[Phase XXIII](XXIII.md) — execution reality (null-PC, present path, audio keep-alive).
