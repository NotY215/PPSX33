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
Future (post-O)            →  RSX pipelines, CPU/SPU accuracy, DMA, HLE, timing, saves, PRX
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
| GUI | Load ELF, decrypt via RPCS3 path, decompile, build, cancel, settings under `%AppData%\PPSX33` |

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
| **Goal** | Stop the guest spinning forever at known null-loop sites (e.g. ~`0x33723c`) so control can reach real cellFs / flip paths |
| **Done** | Stronger soft-break / region skip; GCM flip status cycling; `progress_hw` interaction |
| **Files** | `runtime/ppu_runtime_run.h`, GCM state in `runtime/ps3rt_b.inl` |
| **Logs** | null-loop skip / region exhausted; `[gcm]` flip/wait |

### XX-I — Fuller `cellGcmSys` HLE

| | |
|--|--|
| **Goal** | Replace “always OK / null import” behavior with real configuration, display buffers, and flip lifecycle |
| **Done** | `gcm_hle()` switch; InitBody, GetConfiguration, AddressToOffset, Set/GetDisplayBuffer, flip/wait/reset, map/IO stubs, tile/zcull/handler stubs |
| **Files** | `runtime/ps3rt_b.inl`, `runtime/nid_table.h` |
| **Logs** | `[gcm] XX-I state ready`, `SetDisplayBuffer …` |

### XX-J — PSARC zlib asset streaming

| | |
|--|--|
| **Goal** | Open files inside `.psarc` and stream **decompressed** blocks into guest memory |
| **Done** | TOC (30-byte entries, 40-bit size/offset), block size table, names from entry 0, zlib via `zlib1.dll` / `zlib.dll`, virtual read/seek/stat |
| **Files** | `runtime/assets.cpp` |
| **Logs** | `[psarc] XX-J …`, `[fs] XX-J asset scan`, `virtual open`, `inflate block` |
| **Note** | Place `zlib1.dll` next to the game EXE (or on PATH) for compressed blocks |

### XX-K — SPU ISA interpreter

| | |
|--|--|
| **Goal** | Execute SPU local-store code instead of instant mailbox completion stubs |
| **Done** | Interpreter: load/store, arith/logic, compare, shift, multiply, `selb`/`shufb`, branches, channels (`rdch`/`wrch`), `stop`; initial run slice + tick slices |
| **Files** | `runtime/spu_stub.cpp`, `runtime/ps3rt.cpp`, `runtime/ppu_runtime_core_a.h` |
| **Logs** | `[spu] XX-K run/stop`, unhandled opcodes (subset ISA) |

### XX-L — MFC / DMA + tags

| | |
|--|--|
| **Goal** | Real put/get between LS and main memory; list DMA; barriers; tag completion |
| **Done** | GET/PUT families, GETL/PUTL list walk, barriers/sync, atomic GETLLAR/PUTLLC stubs, tag mask + update HLE |
| **Files** | `runtime/spu_stub.cpp` (MFC path), `ps3rt_spu_mfc_dma` in `ps3rt.cpp` |
| **Logs** | `[spu] XX-L MFC GET/PUT/LIST …` |

### XX-M — SPURS-like scheduling

| | |
|--|--|
| **Goal** | Multi-job / multi-SPU cooperative scheduling and `cellSpurs` HLE |
| **Done** | 32-slot job queue, priority assign to idle SPUs, prio-biased tick slices, `cellSpurs*` NID dispatch, expanded `sys_spu_thread_*` LV2 stubs, **8** SPU contexts |
| **Files** | `runtime/spu_stub.cpp`, `runtime/ps3rt_b.inl`, `runtime/nid_table.h`, `runtime/lv2_syscalls.inl` |
| **Logs** | `[spurs] XX-M schedule …`, `submit …` |

### XX-N — Audio + on-screen status

| | |
|--|--|
| **Goal** | Audible host path + visible title/status while graphics remain incomplete |
| **Done** | `cellAudio` init/port/`AddData`/`Add2chData` → float→s16 → waveOut (48 kHz stereo, 4 buffers); GDI overlay (`GameName | flips=… draws=…`); window title `GameName — PPSX33` |
| **Files** | `runtime/audio_save.cpp`, `runtime/rsx_stub.cpp` |
| **Logs** | `[audio] XX-N …` |

### XX-O — ICON0 window / taskbar icon

| | |
|--|--|
| **Goal** | When running from `USRDIR`, use `PS3_GAME/ICON0.PNG` for **window and taskbar** icons only (not the EXE file icon) |
| **Done** | Multi-path search (`../ICON0.PNG`, `PS3_GAME/…`, case variants); WIC decode → `WM_SETICON` big + small |
| **Files** | `runtime/rsx_stub.cpp` |
| **Logs** | `[rsx] XX-O window/taskbar icon from …`, `window title=…` |

---

## Symptom guide (current)

### Solid blue / black clear with flips

Often: PPU still stuck before meaningful draws; GCM HLE incomplete for a specific call; assets never opened.

**Check logs for:**

1. `[gcm] XX-I` / flip status activity  
2. `[fs]` / `[psarc] XX-J` opens after null-loop skip  
3. `[spu]` / `[spurs]` if the title waits on SPU jobs  

### No sound

- Confirm `[audio] XX-N cellAudioInit` / `AddData`  
- Host waveOut device available  

### No ICON0

- EXE must run from a layout where `../ICON0.PNG` or `PS3_GAME/ICON0.PNG` resolves  
- Log line `XX-O window/taskbar icon from` confirms success  

### Build / link errors

Prefer clean rebuild after dropping a full `runtime/` set. Avoid mixing half-applied phase zips.

---

## Recommended workflow

1. Accept legal notice (TERMS open in system browser).  
2. Locate RPCS3 if decrypting EBOOT (button reappears if missing on next launch).  
3. Load decrypted ELF (file dialog remembers last path).  
4. Decompile / lift chunks.  
5. Set controller map **before** Build (locked for that build).  
6. Build → game-named EXE + `ps3rt.dll` + `guest_image.bin` in `output/`.  
7. **Copy to game folder** → files land next to `EBOOT.BIN` under `USRDIR`.  
8. Run the host EXE from `USRDIR` (so ICON0 and relative asset paths work).  
9. On issues: attach the **latest** run log via the professional issue templates.

---

## Future work (post XX-O) — missing / weak areas blocking real play

The milestones above describe code paths present in the tree, not complete or accurate PS3 hardware emulation. The following gaps remain open and are planned for implementation. Statuses are intentionally **PLANNED** or **PARTIAL** until the relevant semantics are implemented and validated with tests and real title traces.

| Area | Status | Missing / weak implementation | Why it blocks real play | Planned implementation direction |
|------|--------|-------------------------------|--------------------------|----------------------------------|
| **RSX graphics** | PARTIAL | Real surface uploads; shader handling; textured draws; vertex/fragment pipelines; depth/stencil; render targets beyond clear/flip | Many titles remain on solid blue/black clears or show no meaningful scene | Implement surface and texture upload paths, draw-state decoding, shader/pipeline support, depth/stencil and render-target transitions; validate with captured commands and visual test scenes |
| **PPU accuracy** | PARTIAL | A number of instructions are approximate or no-op; recovery often skips a region instead of reproducing correct semantics | Incorrect branches, numerical results, state changes and silent logic failures | Audit opcode coverage, implement missing semantics and edge cases, make unsupported instructions explicit, and add instruction-level regression tests; keep region skipping diagnostic rather than a correctness substitute |
| **SPU ISA** | PARTIAL | Current subset focuses on load/store, basic arithmetic, branches, channels and stop; broad vector/floating-point coverage, shuffle edge cases and complete channel behavior are missing | SPU-driven physics, audio, decompression and job work can hang or produce incorrect output | Expand opcode families and exact lane/rounding/condition semantics; model channel behavior and add focused interpreter tests |
| **MFC / DMA fidelity** | PARTIAL | List-element formats are incomplete; stall handling and queue depth are not accurate enough | DMA races, tag waits, ordering problems and memory corruption can deadlock or destabilize titles | Complete list formats and command validation; model queues, stalls, tags, ordering and completion transitions; add deterministic DMA/tag tests |
| **SPURS** | PARTIAL | Scheduler is a simplified priority queue, not full job-chain/taskset behavior | Titles using more complex SPURS patterns may never complete jobs or unblock waiting threads | Implement job-chain/taskset lifecycle, workload readiness, priorities, event/wait behavior and completion signaling; test multi-job and multi-SPU cases |
| **cellFs / VFS** | PARTIAL | Basic open/read and PSARC support exist, but directory listing, error codes, path semantics and encrypted/SELF-related paths are weak or unsupported | Assets may be missing, path resolution may fail, and games may abort on unexpected filesystem results | Expand directory/stat/seek/read behavior, path normalization, error mapping and archive edge cases; clearly report unsupported encrypted/SELF flows |
| **Audio** | PARTIAL | Port + AddData to waveOut works, but ring-buffer layout, event queues and multi-port timing are incomplete | Audio may crackle, become silent, use wrong channels or drift out of sync | Model per-port buffers and status, queue/event semantics, channel formats and timing; add buffer-boundary and multi-port tests |
| **LV2 / syscalls** | PARTIAL | Many syscall paths remain stubs or return success without performing the requested behavior | Threads, events, semaphores, synchronization and memory management can break despite apparent success | Inventory syscall usage, implement correct return/error/state semantics for high-impact calls, and ensure unimplemented calls fail visibly instead of falsely succeeding |
| **Libraries / HLE** | PARTIAL | Only slices of cellGcmSys, cellSpurs, cellAudio and early cellFs are implemented; much of libc, libpad, save-data and network behavior is missing | Games can import hundreds of NIDs that currently resolve to null or stub behavior | Prioritize NIDs from real game traces, implement library-specific contracts and error codes, and add NID dispatch/behavior tests |
| **Save data** | PLANNED | Save-data API is minimal or stubbed | Titles cannot reliably create, enumerate, load or persist progress | Implement save-data lifecycle and metadata handling with a user-selected host save directory; test create/load/update/error cases |
| **Pad / controller** | PARTIAL | XInput mapping works, but pressure-sensitive buttons, Sixaxis data and full libpad semantics are incomplete | Games requiring analog pressure, motion data or richer controller status may misread input | Extend pad state and API responses, map available host inputs where possible, and return explicit unsupported status for unavailable hardware features |
| **Timing / threads** | PARTIAL | Scheduling is cooperative-like, without an accurate coordinated PPU/SPU/RSX timing model | Races, speed anomalies, missed wakeups and softlocks | Introduce explicit guest timing and synchronization points across PPU, SPU, DMA and RSX; validate wait/signal behavior and repeatability |
| **PRX / modules** | PARTIAL | Module detection exists, but real PRX loading and relocation are not robust | Titles that depend on dynamically loaded modules may fail during startup or later execution | Implement module metadata parsing, supported relocation types, import/export resolution and module lifecycle; report unsupported relocation/import cases clearly |

### Implementation and validation policy

1. Treat each row above as an open work area, not as completed functionality.
2. Prioritize by evidence from game logs, import/NID traces, RSX command traces and reproducible minimal tests, rather than by static instruction counts alone.
3. For each implementation, add targeted tests for normal behavior, edge cases and failure paths before changing the status from **PLANNED/PARTIAL**.
4. Keep known limitations visible in logs and compatibility notes. A stub returning success, a skipped PPU region, or a clear/flip-only RSX path must not be described as full support.
5. Update the relevant runtime documentation and per-game notes as behavior is verified. Static-lift coverage does not prove correct execution or playability.

---

## Related docs

| Doc | Purpose |
|-----|----------|
| [README.md](README.md) | Project intro, status table, Alpha package |
| [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) | Build, runtime layout, smoke tests |
| [docs/PPU_COVERAGE.md](docs/PPU_COVERAGE.md) | Static lift coverage notes |
| [docs/KEY_MAPPING.md](docs/KEY_MAPPING.md) | Controller mapping |
| [docs/games/README.md](docs/games/README.md) | Per-title analysis records |
| [LEGAL.md](LEGAL.md) / [TERMS.md](TERMS.md) | Legal notices |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| [SUPPORT.md](SUPPORT.md) | Support channels |

---

## Reporting issues

Use the repository issue templates (SPU coverage, launch, delays, other). Always attach the **newest** log from the game folder or outside-folder launch. Incomplete SPU/ISA reports help prioritise interpreter gaps.

---

*PPSX33 does not include, use, or distribute proprietary decryption keys or copyrighted game assets. You must supply your own legally obtained dumps and toolchains.*
