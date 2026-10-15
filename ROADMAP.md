# PPSX33 Roadmap

**Last updated:** 2026-10-15  
**Scope:** Runtime playability, packaging, and host tooling for the experimental PS3 static recompiler.

PPSX33 is a **research toolkit**, not a retail emulator or a finished port of any commercial game. Milestone labels describe engineering work completed in-tree. They do **not** claim verified boot, full graphics, audio fidelity, or playability for any title.

---

## How to read this document

| Label | Meaning |
|-------|---------|
| **DONE** | Code landed in `runtime/` / GUI / docs for this milestone |
| **TESTING** | Present and usable, but regressions and incomplete semantics are expected |
| **PARTIAL** | Core path exists; edge cases, accuracy, or coverage still missing |
| **PLANNED** | Described for future work; not a commitment |

---

## Track overview

```text
Tooling & lift (I–XIX)           → static analysis, decompile, project build UI
Package (XX-A … XX-G)            → game-named EXE, copy-to-USRDIR, legal, logs
Runtime playability (XX-H … O)   → GCM, cellFs/PSARC, SPU/MFC/SPURS base, audio, ICON0
Post-O lettered (A … E)          → RSX, PPU accuracy, SPU ISA, MFC, SPURS
Future (remaining)               → shaders/raster, cellFs dirs, audio ring, LV2, saves, PRX
```

---

## Phase XX-H … XX-O — runtime playability (DONE in tree)

Base runtime: null-loop/GCM, cellGcmSys HLE, PSARC zlib, SPU interpreter core, MFC put/get, SPURS queue, cellAudio, ICON0. See prior roadmap revisions for per-phase file maps.

---

## Future work (post XX-O)

### Lettered implementations (post XX-O)

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **A** | RSX surfaces + color blit | **DONE** | Surface descriptors (color/depth offset, pitch, format, size), texture unit 0, expanded NV40 methods, guest color-buffer blit to D3D11 on flip. Files: `runtime/rsx_stub.cpp`. Logs: `[rsx] A …` |
| **B** | PPU accuracy helpers | **DONE** | mulhd/mulhdu, div edge cases, rlwinm/rld*, addc/adde/subfc + XER[CA], FPSCR sticky, CR0 helpers, opcode decoder; recovery prefers LR/stack over region-skip. Files: `ppu_runtime_core_a.h`, `core_b.h`, `ppu_runtime_run.h`. Logs: `[ppu] B …` |
| **C** | SPU ISA expansion | **DONE** | or/orx/nand/eqv, cg/cgt/ceq, rotqby, fa/fs/fm/frest, absdb/sumb; unhandled tagged `[spu] C`. Files: `runtime/spu_stub.cpp`. Logs: `[spu] C …` |
| **D** | MFC / DMA fidelity | **DONE** | List element size16+EA48, nbytes-0 end, list stall flag, RdTagStat = completed∩mask, WrTagUpdate modes, queue depth max 16. Files: `runtime/spu_stub.cpp`. Logs: `[spu] D …` |
| **E** | SPURS jobchain | **DONE** | Job `ready` flag, `chain_next`, completion cause, `ps3rt_spurs_link_chain` / `set_ready`, schedule only ready jobs, kick successor on done. Files: `runtime/spu_stub.cpp`. Logs: `[spurs] E …` |

### Remaining gaps

| Sno. | Name | Status | Work |
|------|------|--------|------|
| 1 | RSX shaders / raster | PARTIAL | Beyond A: vertex fetch, host shaders, depth test on draws, multi-RT, more formats |
| 2 | PPU lifter wiring | PARTIAL | Beyond B: emit B helpers from lifter; VMX/FP completeness; instruction tests |
| 3 | cellFs / VFS | PARTIAL | Directories, error codes, path edge cases; report unsupported SELF |
| 4 | Audio ring / events | PARTIAL | Per-port buffers, event queues, multi-port timing |
| 5 | LV2 / syscalls | PARTIAL | High-impact threads/events/memory; fail visibly when unimplemented |
| 6 | Libraries / HLE | PARTIAL | Trace-driven NID coverage beyond Gcm/Spurs/Audio/Fs slices |
| 7 | Save data | PLANNED | Host save directory lifecycle |
| 8 | Pad / Sixaxis | PARTIAL | Pressure, motion, full libpad |
| 9 | Timing / threads | PARTIAL | Coordinated PPU/SPU/RSX timing model |
| 10 | PRX / modules | PARTIAL | Load, relocate, import/export lifecycle |

### Implementation policy

1. Lettered rows are **DONE in tree** for the listed work — not full hardware parity.
2. Remaining rows stay open until lettered or otherwise completed.
3. Prefer evidence from logs, NID traces, and minimal tests over static instruction counts.
4. Keep limitations visible in logs.

---

## Symptom guide

- Solid blue: check `[gcm]`, `[psarc]`, `[rsx] A COLOR_OFFSET` / `blits=`
- SPU stalls: `[spu] C unhandled`, `[spu] D MFC`, `[spurs] E`
- No sound: `[audio] XX-N`
- No ICON0: run from USRDIR; `XX-O` log line

---

## Related docs

| Doc | Purpose |
|-----|----------|
| [README.md](README.md) | Status table, lettered summary |
| [USER_MANUAL.md](USER_MANUAL.md) | End-user workflow |
| [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) | Architecture |

---

*PPSX33 does not include proprietary keys or copyrighted game assets.*
