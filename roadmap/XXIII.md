# Phase XXIII — Execution reality (ACTIVE)

GOW III log (`2026-10-15_20-06-04`): PSARC scans OK, waveOut opens, flips advance to 7000+, but **methods stay at 4 / draws≈1** from synthetic GCM inject — guest FIFO PUT is still 0. Overlay can show `draws=0` (per-frame) while lifetime draws count injects. Rendering of the game scene has **not** started.

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **XXIII-A** | DONE | Null-PC recovery | `ppu_runtime_run.h` |
| **XXIII-B** | DONE | Present every flip | `rsx_stub.cpp` |
| **XXIII-C** | DONE | Audio host keep-alive | `audio_save.cpp` |
| **XXIII-D** | DONE | PPU opcode markers | lifter |
| **XXIII-E** | DONE | RSX present + FIFO methods | `rsx_stub.cpp` |
| **XXIII-F** | DONE | SPU/MFC tag/stall | `spu_stub.cpp` |
| **XXIII-G** | DONE | LV2 wait/signal contracts | `lv2_syscalls.inl` |
| **XXIII-H** | DONE | PRX NID recovery | `ps3rt_b.inl` |
| **XXIII-I** | DONE | cellFs / PSARC miss logging | `assets.cpp` |
| **XXIII-J** | DONE | SPURS job slice + chain ready | `spu_stub.cpp` |
| **XXIII-K** | DONE | Timing: faster RSX assist, GET=PUT | `ps3rt_b.inl` |
| **XXIII-L** | PLANNED | Coverage metrics (semantic vs decode) | docs |
| **XXIII-M** | PLANNED | Guest GCM FIFO: real PUT/GET/command stream | `ps3rt_b.inl`, `rsx_stub.cpp` |
| **XXIII-N** | PLANNED | Vertex buffers / attributes / draw decode | `rsx_stub.cpp` |
| **XXIII-O** | PLANNED | Fragment/vertex program load & host shaders | `rsx_stub.cpp` |
| **XXIII-P** | PLANNED | Texture formats, pitch, upload correctness | `rsx_stub.cpp` |
| **XXIII-Q** | PLANNED | Import/HLE coverage for init that issues draws | `ps3rt_b.inl`, `nid_table.h` |
| **XXIII-R** | PLANNED | PPU control-flow (wild-pc / TOC / OPD) | `ppu_runtime_run.h` |
| **XXIII-S** | PLANNED | Threading: workers that fill FIFO | LV2 + runtime |

## Why the screen is still solid

1. Guest **never advances GCM PUT** (log: inject path, methods=4 forever).
2. Without guest commands there are **no textures/shaders** (`shaders=0 tex=0`).
3. XXIII-E clear pulse / blit only affects empty or synthetic frames — not game geometry.
4. Import stubs still dominate boot (`mod=?`, kind=0) — init that would fill the FIFO may not run.

**I/J/K** improve FS visibility, SPURS completion, and host assist cadence; they do **not** by themselves start real 3D. That is **M–Q**.

## Log tags

- `[fs] XXIII-I miss` — path map / open failure
- `[spurs] XXIII-J schedule` — job slice / done
- `[gcm] XXIII-K guest put/get` — when guest writes FIFO
- `[time] XXIII-K` — PPU/SPU/RSX phase
