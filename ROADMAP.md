# PPSX33 Roadmap

**Last updated:** 2026-10-15  
**Scope:** Runtime playability and post–XX-O lettered implementations.

PPSX33 is a **research toolkit**, not a retail emulator. Milestone labels describe in-tree engineering work, not verified commercial playability.

---

## Track overview

```text
XX-A … XX-O     → packaging + base runtime (GCM, PSARC, SPU/MFC/SPURS core, audio, ICON0)
A … H           → lettered post-O implementations (see table)
Remaining       → shaders/raster, lifter wiring, deeper HLE, saves, PRX, timing
```

---

## Future work (post XX-O)

### Lettered implementations (post XX-O)

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **A** | RSX surfaces + blit | **DONE** | COLOR_OFFSET/pitch tracking; flip path tagged `[rsx] A`. `runtime/rsx_stub.cpp` |
| **B** | PPU accuracy helpers | **DONE** | mulhd/mulhdu, div edge cases, rlwinm; LR-first recovery. `ppu_runtime_core_a.h`, `ppu_runtime_run.h` |
| **C** | SPU ISA expansion | **DONE** | or/orx/nand/eqv, cg/cgt/ceq, rotqby, fa/fs/fm/frest. `runtime/spu_stub.cpp` · `[spu] C` |
| **D** | MFC / DMA fidelity | **DONE** | List size16+EA48, tag poll = completed∩mask, queue depth 16. · `[spu] D` |
| **E** | SPURS jobchain | **DONE** | ready, chain_next, link_chain / set_ready. · `[spurs] E` |
| **F** | cellFs directories | **DONE** | opendir / readdir / closedir, host listing. `runtime/assets.cpp` · `[fs] F` |
| **G** | Audio ring / notify | **DONE** | Ring write/read indices, notify key, port status. `runtime/audio_save.cpp` · `[audio] G` |
| **H** | LV2 syscalls expand | **DONE** | mutex, event queue, semaphore, PPU thread, memory, fs syscall numbers. `runtime/lv2_syscalls.inl` |

Apply package: **`PPSX33_A-F_G_H.zip`** (includes A–H runtime files).

### Remaining gaps

| Sno. | Name | Status | Work |
|------|------|--------|------|
| 1 | RSX shaders / raster | PARTIAL | Vertex fetch, host shaders, depth on draws, multi-RT |
| 2 | PPU lifter wiring | PARTIAL | Emit B helpers from lifter; VMX/FP tests |
| 3 | Audio multi-port fidelity | PARTIAL | Beyond G: full block layout, event delivery |
| 4 | Libraries / HLE | PARTIAL | Trace-driven NID coverage |
| 5 | Save data | PLANNED | Host save lifecycle |
| 6 | Pad / Sixaxis | PARTIAL | Pressure, motion |
| 7 | Timing / threads | PARTIAL | Coordinated PPU/SPU/RSX timing |
| 8 | PRX / modules | PARTIAL | Load, relocate, imports |

---

## Related docs

[README.md](README.md) · [USER_MANUAL.md](USER_MANUAL.md) · [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md)

---

*No proprietary keys or copyrighted game assets are included.*
