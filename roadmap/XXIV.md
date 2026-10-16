# Phase XXIV — Boot-to-first-pixels (ACTIVE)

Driven by GOW III log `2026-10-16_14-13-47`: audio storm, FIFO put=get=0x20 with inflated methods, missing HLSL/diagnostic proof.

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **XXIV-A** | DONE (untested) | Audio: `waveOutOpen` fail-once; no per-tick retry/log | `runtime/audio_save.cpp` |
| **XXIV-B** | DONE (untested) | FIFO: skip empty/tiny delta; bound drain; per-flip method count; put≤0x20 → inject only | `runtime/rsx_stub.cpp`, `runtime/ps3rt_b.inl` |
| **XXIV-C** | DONE (untested) | HLSL compile log once; diagnostic gradient when no guest tex; ensure_hlsl at init | `runtime/rsx_stub.cpp` |
| **XXIV-D** | PLANNED | PRX NID quality + import return-to-LR | `ps3rt_b.inl`, `nid_table.h` |
| **XXIV-E** | PLANNED | COLOR_OFFSET blit every flip when set | `rsx_stub.cpp` |

## Expected log after A–C

- At most **one** `[audio] XXIV-A waveOutOpen failed` (or one success).
- `[rsx] XXIV-C HLSL blit VS/PS ready` **or** `XXIV-C D3DCompile DLL missing` / compile fail.
- `[rsx] XXIV-C diagnostic frame flip=…` when guest tex still empty.
- Flip lines `methods=` stay small unless guest `put` advances.

## Related

Prior surface: [roadmap/XXIII.md](XXIII.md). Overview: [ROADMAP.md](../ROADMAP.md).
