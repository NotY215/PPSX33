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
| **XXII-G** | RSX draw execution | PLANNED | `runtime/rsx_stub.cpp` | Beyond clear/blit: vertex/texture sample path depth |
| **XXII-H** | PPU instruction depth | PLANNED | lifter + `ppu_runtime_*` | CR/XER/FPSCR edge cases; less recovery dependence |
| **XXII-I** | SPU vector/float depth | PLANNED | `runtime/spu_stub.cpp` | Wider ISA; channel events |
| **XXII-J** | LV2 contracts | PLANNED | `lv2_syscalls.inl` | Real wait/signal semantics for high-impact calls |
| **XXII-K** | Save / pad depth | PLANNED | `audio_save.cpp`, pad paths | Save lifecycle + Sixaxis/pressure consumers |
| **XXII-L** | OpenGL probe prep | PLANNED | build + runtime | Detect D3D11 suitability; stub OpenGL 3.3/2.1 selection (future) |

---

## XXII-E detail (this drop)

- `PrxMod` / exports declared **before** load helpers (fixes C3861 / C2065).
- `prx_mod_load` → `prx_load_file_into_guest(path, base, &entry)` three-arg API.
- Host search: `.`, `USRDIR`, `USRDIR/modules`, `modules` (+ `.prx`).
- ELF64 BE `PT_LOAD` into guest RAM; **SELF** not decrypted (log + fail).
- Broken log string (`"%s"` splice) fixed so MSVC parses the file.

## XXII-F detail (this drop)

- Host frame-pace helper remains on flip-assist path (`frame_pace_wait`).
- Audio source tagged for ring/notify surface continuity.

---

## Symptom guide (XXII)

| Symptom | Check |
|---------|--------|
| C3861 `prx_mod_load` | Ensure this `ps3rt_b.inl` order (registry before try-load) |
| SELF / module missing | `[prx] SELF not loaded` or path search logs |
| Silent unknown libs | `[prx] XXII-D unknown nid=` |
| Solid clear only | XXII-A upload/blit; later XXII-G draws |
| Timing odd | `[time] X` / frame pace |

---

## Future after XXII

OpenGL **3.3** (or **2.1**) auto-fallback when D3D11 is unsuitable — see main [ROADMAP.md](../ROADMAP.md).

## Related

[ROADMAP.md](../ROADMAP.md) · [XXI.md](XXI.md)
