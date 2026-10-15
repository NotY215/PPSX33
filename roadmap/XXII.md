# Phase XXII — Pipeline correctness (ACTIVE)

**Track:** PHASE 1 after XXI (lettered A–Z)  
**Goal:** Reduce HLE lies and hard-stops called out in pipeline review (RSX, PPU/SPU, LV2, modules/FS, timing).  
**Not a claim:** commercial playability or full hardware accuracy.

---

## Subphase index

| ID | Name | Status | Primary files | Work |
|----|------|--------|---------------|------|
| **XXII-A** | RSX present / color-buffer | DONE | `runtime/rsx_stub.cpp` | Prefer guest color/tex on present; host upload/blit path; `[rsx] XXII-A` |
| **XXII-B** | PPU soft-recovery | DONE | `runtime/ps3rt_b.inl`, `ppu_runtime_*.h` | Unimplemented ops soft-continue; shared lifter helpers |
| **XXII-C** | SPU / MFC fidelity | DONE | `runtime/spu_stub.cpp` | Tag/stall/mach-status surface less always-zero |
| **XXII-D** | LV2 / HLE visibility | DONE | `runtime/ps3rt_b.inl`, `lv2_syscalls.inl` | Unknown NID / stub first-hit logging |
| **XXII-E** | Modules / PRX / FS | DONE | `runtime/ps3rt_b.inl`, `runtime/assets.cpp` | Ordered PRX registry + ELF/raw load; path search; SELF skip; FS notes |
| **XXII-F** | Timing / audio surface | DONE | `runtime/ps3rt_b.inl`, `runtime/audio_save.cpp` | Frame pacing helper; audio ring/notify surface |
| **XXII-G** | RSX draw execution | DONE | `runtime/rsx_stub.cpp` | Beyond clear/blit: vertex/texture sample path depth |
| **XXII-H** | PPU instruction depth | DONE | lifter + `ppu_runtime_*` | CR/XER/FPSCR edge cases; less recovery dependence |
| **XXII-I** | SPU vector/float depth | DONE | `runtime/spu_stub.cpp` | Wider ISA; channel events |
| **XXII-J** | LV2 contracts | DONE | `lv2_syscalls.inl` | Real wait/signal semantics for high-impact calls |
| **XXII-K** | Save / pad depth | Done | `audio_save.cpp`, pad paths | Save lifecycle + Sixaxis/pressure consumers |
| **XXII-L** | OpenGL probe prep | DONE | build + runtime | Detect D3D11 suitability; stub OpenGL 3.3/2.1 selection (future) |

---

## Future after XXII

OpenGL **3.3** (or **2.1**) auto-fallback when D3D11 is unsuitable — see main [ROADMAP.md](../ROADMAP.md).

## Related

[ROADMAP.md](../ROADMAP.md) · [XXI.md](XXI.md)