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
Tooling & lift (I–XIX)     →  static analysis, decompile, project build UI
Package (XX-A … XX-G)      →  game-named EXE, copy-to-USRDIR, legal, logs
Runtime playability (XX-H … XX-O)  →  GCM, cellFs/PSARC, SPU/MFC/SPURS, audio, ICON0
Post-O lettered (A, B, …)  →  RSX surfaces/blit (A), PPU accuracy helpers (B), …
Future (remaining)         →  shaders, SPU/DMA/SPURS, HLE, timing, saves, PRX
```

---

## Phases I–XIX — core pipeline (TESTING)

Foundation work: ELF load, PPU static lift, HLE stubs, RSX host present path, pad/audio hooks, MSVC build orchestration, WinForms workflow.

| Theme | What exists |
|-------|-------------|
| ELF / OPD | Loadable segments, entry descriptors, analysis reports |
| PPU lift | Chunk-based translation; reported “unimplemented” counts are static, not semantic proof |
| Runtime shell | `ps3rt.dll`, guest image, progress stages (BOOT → MEM → GCM → RSX → PPU → SPU) |
| RSX host | D3D11 clear/flip present; method decode incomplete |
| Pad | XInput + `pad_map.json`; log only when game window focused |
| GUI | Load ELF, decrypt via RPCS3 path, decompile, build, cancel, settings under `%AppData%\\PPSX33` |

These phases remain **testing** quality. Instruction counts in README game tables are **static-lift snapshots**, not playability metrics.

---

## Phase XX-A … XX-G — packaging & host UX (DONE)

| ID | Status | Summary |
|----|--------|----------|
| **XX-A** | DONE | Game-named EXE + `ps3rt.dll` + `guest_image.bin` + `pad_map.json` |
| **XX-B** | DONE | cellFs path mapping + early PSARC TOC (later expanded in XX-J) |
| **XX-C** | DONE | Copy output into `PS3_GAME/USRDIR` (auto-find `EBOOT.BIN`) |
| **XX-D** | DONE | Controller mapping persisted as `pad_map.json` |
| **XX-E** | DONE | Legal / TERMS first-run; links open in the **system browser** |
| **XX-F** | DONE | Post-build layout: reports at project root; runtime binaries under `output/` |
| **XX-G** | DONE | Named run logs; pad/XInput log only while the game window is focused |

### Expected layout after build + copy

```text
<GameFolder>/
  analysis / lift / build log (project side)
  PS3_GAME/USRDIR/
    EBOOT.BIN
    <GameName>.exe      (or PPSX33-named host EXE)
    ps3rt.dll
    guest_image.bin
    pad_map.json
  PS3_GAME/ICON0.PNG    (used by XX-O for window/taskbar icon)
```

---

## Phase XX-H … XX-O — runtime playability (DONE in tree)

Implemented across `runtime/` (interpreter, HLE, assets, RSX host). Apply the full set together when updating a checkout.

### XX-H — Escape null-loop / soft GCM

| | |
|--|--|
| **Goal** | Stop the guest spinning forever at known null-loop sites |
| **Done** | Stronger soft-break / region skip; GCM flip status cycling; `progress_hw` interaction |
| **Files** | `runtime/ppu_runtime_run.h`, GCM state in `runtime/ps3rt_b.inl` |
| **Logs** | null-loop skip / region exhausted; `[gcm]` flip/wait |

### XX-I — Fuller `cellGcmSys` HLE

| | |
|--|--|
| **Goal** | Real configuration, display buffers, flip lifecycle |
| **Done** | `gcm_hle()` switch; InitBody, GetConfiguration, AddressToOffset, Set/GetDisplayBuffer, flip/wait/reset, map/IO stubs |
| **Files** | `runtime/ps3rt_b.inl`, `runtime/nid_table.h` |
| **Logs** | `[gcm] XX-I state ready` |

### XX-J — PSARC zlib asset streaming

| | |
|--|--|
| **Goal** | Stream decompressed PSARC blocks into guest memory |
| **Done** | TOC, block table, zlib via `zlib1.dll` |
| **Files** | `runtime/assets.cpp` |
| **Logs** | `[psarc] XX-J …` |

### XX-K — SPU ISA interpreter

| | |
|--|--|
| **Goal** | Execute SPU local-store code |
| **Done** | load/store, arith, branches, channels, stop |
| **Files** | `runtime/spu_stub.cpp`, `runtime/ps3rt.cpp` |
| **Logs** | `[spu] XX-K …` |

### XX-L — MFC / DMA + tags

| | |
|--|--|
| **Goal** | Real put/get, list DMA, tags |
| **Done** | GET/PUT families, list walk, barriers, tag mask |
| **Files** | `runtime/spu_stub.cpp` |
| **Logs** | `[spu] XX-L MFC …` |

### XX-M — SPURS-like scheduling

| | |
|--|--|
| **Goal** | Multi-job / multi-SPU scheduling |
| **Done** | 32-slot job queue, 8 SPU contexts, cellSpurs HLE |
| **Files** | `runtime/spu_stub.cpp`, `runtime/ps3rt_b.inl` |
| **Logs** | `[spurs] XX-M …` |

### XX-N — Audio + on-screen status

| | |
|--|--|
| **Goal** | Host audio + visible status |
| **Done** | cellAudio → waveOut; GDI overlay; window title |
| **Files** | `runtime/audio_save.cpp`, `runtime/rsx_stub.cpp` |
| **Logs** | `[audio] XX-N …` |

### XX-O — ICON0 window / taskbar icon

| | |
|--|--|
| **Goal** | ICON0.PNG as window/taskbar icon |
| **Done** | Multi-path search; WIC → WM_SETICON |
| **Files** | `runtime/rsx_stub.cpp` |
| **Logs** | `[rsx] XX-O …` |

---

## Symptom guide (current)

### Solid blue / black clear with flips

Check: `[gcm] XX-I`, `[psarc] XX-J`, `[spu]`/`[spurs]`, `[rsx] A COLOR_OFFSET` / `blits=`.

### No sound

Confirm `[audio] XX-N` and waveOut device.

### No ICON0

Run from USRDIR; look for `XX-O window/taskbar icon from`.

### Build / link errors

Clean rebuild after applying a full `runtime/` set.

---

## Recommended workflow

1. Accept TERMS. 2. Load decrypted ELF. 3. Decompile. 4. Map controls before Build. 5. Build. 6. Copy to USRDIR. 7. Run from USRDIR. 8. Attach latest log on issues.

---

## Future work (post XX-O) — missing / weak areas blocking real play

The milestones above describe code paths present in the tree, not complete or accurate PS3 hardware emulation. The following gaps remain open. Statuses are **PLANNED**, **PARTIAL**, or advanced by lettered implementations **A / B / …** when code has landed.

### Lettered implementations (post XX-O)

| ID | Status | Summary | Files / log tags |
|----|--------|---------|------------------|
| **A** | DONE in tree | RSX surface descriptors (color/depth offset, pitch, format, size), texture unit 0 state, expanded NV40 method decode, guest color-buffer blit into D3D11 on flip | `runtime/rsx_stub.cpp` · `[rsx] A …` |
| **B** | DONE in tree | PPU accuracy helpers: mulhd/mulhdu, divd/divdu edge cases, rlwinm/rldicl/rldicr, addc/adde/subfc + XER[CA], FPSCR sticky notes, CR0 helpers, opcode decoder; recovery prefers LR/stack over region-skip | `runtime/ppu_runtime_core_a.h`, `core_b.h`, `ppu_runtime_run.h` · `[ppu] B …` |

### Remaining gaps

| Area | Status | Missing / weak implementation | Why it blocks real play | Planned implementation direction |
|------|--------|-------------------------------|--------------------------|----------------------------------|
| **RSX graphics** | PARTIAL (A) | Surface upload + color blit exist; full shader pipelines, textured triangle raster, depth/stencil test in draws, multi-RT still missing | Many titles remain blue/black or show only blitted surfaces without correct scene geometry | Continue from A: vertex fetch, shader bind→host, depth test on draws, more format conversions |
| **PPU accuracy** | PARTIAL (B) | Helpers for mul/div/rotate/carry landed; lifted code must call them; many ops still approximate in generated chunks | Incorrect branches, numerical results, silent logic failures | Wire B helpers into the lifter; expand VMX/FP; instruction-level tests; keep region-skip diagnostic only |
| **SPU ISA** | PARTIAL | Subset load/store, arith, branches, channels, stop; broad vector/FP and shuffle edge cases missing | SPU physics/audio/decode can hang or miscompute | Expand opcode families; channel fidelity; interpreter tests |
| **MFC / DMA fidelity** | PARTIAL | List formats incomplete; stall/queue depth not accurate | DMA races, tag waits, memory corruption | Complete list formats; model queues/stalls/tags |
| **SPURS** | PARTIAL | Simple priority queue, not full job-chain/taskset | Complex SPURS titles never finish jobs | Job-chain lifecycle, readiness, events |
| **cellFs / VFS** | PARTIAL | open/read + PSARC; weak directories, error codes, SELF paths | Missing assets, failed loads | Directory/stat/error mapping; report unsupported SELF |
| **Audio** | PARTIAL | Port + AddData → waveOut; ring/events/multi-port incomplete | Crackling, silence, drift | Per-port buffers, events, timing tests |
| **LV2 / syscalls** | PARTIAL | Many stubs return success without real behaviour | Threads, events, memory management break | High-impact syscalls first; fail visibly when unimplemented |
| **Libraries / HLE** | PARTIAL | Slices of GcmSys, Spurs, Audio, cellFs; libc/libpad/save/network mostly missing | Hundreds of NIDs are null/stub | Trace-driven NID priority |
| **Save data** | PLANNED | Minimal/stub | No reliable progress load/save | Host save directory lifecycle |
| **Pad / controller** | PARTIAL | XInput map works; pressure/Sixaxis/libpad incomplete | Some games misread input | Extend pad state; explicit unsupported features |
| **Timing / threads** | PARTIAL | Cooperative; no coordinated PPU/SPU/RSX timing | Races, softlocks, speed issues | Explicit guest timing points |
| **PRX / modules** | PARTIAL | Detection exists; load/reloc not robust | Module-dependent titles fail | Relocations, import/export, lifecycle |

### Implementation and validation policy

1. Treat each row as open work unless a lettered implementation explicitly advances it.
2. Prioritize by game logs, NID traces, RSX command traces, and minimal tests — not static instruction counts alone.
3. Add targeted tests for normal, edge, and failure paths before promoting status.
4. Keep limitations visible in logs. A clear/flip path or diagnostic region-skip is not full support.
5. Update runtime docs and per-game notes as behaviour is verified.

---

## Related docs

| Doc | Purpose |
|-----|----------|
| [README.md](README.md) | Project intro, status table |
| [USER_MANUAL.md](USER_MANUAL.md) | End-user workflow, legal, troubleshooting |
| [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) | Build, runtime layout |
| [docs/PPU_COVERAGE.md](docs/PPU_COVERAGE.md) | Static lift coverage |
| [LEGAL.md](LEGAL.md) / [TERMS.md](TERMS.md) | Legal notices |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| [SUPPORT.md](SUPPORT.md) | Support channels |

---

## Reporting issues

Use the repository issue templates. Always attach the **newest** run log.

---

*PPSX33 does not include, use, or distribute proprietary decryption keys or copyrighted game assets. You must supply your own legally obtained dumps and toolchains.*
