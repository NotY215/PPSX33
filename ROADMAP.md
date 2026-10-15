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
Future (post-O)            →  accuracy, coverage, more HLE, RSX draws
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

## Future work (post XX-O) — PLANNED / PARTIAL

These are known compatibility gaps that remain before PPSX33 can be considered broadly playable. They are **planned implementation work**, not claims that the features already work. Prioritize correctness and observable title progress over superficial boot success.

### Playability blockers and implementation plan

| Area | Status | Missing / weak behaviour to implement | Why it blocks real play |
|------|--------|----------------------------------------|-------------------------|
| **RSX graphics** | PLANNED · highest priority | Real surface and texture uploads; shader handling; textured draws; vertex/fragment pipeline; depth/stencil; render targets beyond clear/flip; more complete GCM command and state handling | Many titles remain on a solid blue/black clear because actual scene geometry and materials are not rendered. |
| **PPU accuracy** | PARTIAL | Replace approximate or no-op instruction semantics with correct behaviour; improve branch and arithmetic accuracy; make fault recovery identify and report unsupported regions instead of relying on region skipping | Incorrect branches and calculations can derail game logic, while silent skips hide the original failure. |
| **SPU ISA** | PARTIAL | Expand beyond the current subset with missing vector and floating-point instructions, instruction edge cases, shuffle semantics, and complete channel behaviour | Physics, audio, decompression, and other SPU-heavy tasks may hang or produce incorrect results. |
| **MFC / DMA fidelity** | PARTIAL | Complete list-element formats and transfer validation; model stall notifications, tag status, queue depth, ordering, and synchronization accurately | Incorrect DMA completion and tag waits can create races, memory corruption, or deadlocks. |
| **SPURS** | PARTIAL | Implement more faithful job-chain, workload, taskset, synchronization, and scheduling semantics instead of relying only on a simple priority queue | Titles using complex SPURS job patterns may never complete their work. |
| **cellFs / virtual filesystem** | PARTIAL | Improve directory enumeration, path resolution, file metadata and error codes; handle supported encrypted/container/SELF-related paths only where legally and technically feasible; strengthen PSARC edge cases | Missing files, bad paths, and incorrect errors prevent assets and configuration from loading. |
| **Audio** | PARTIAL | Correct port/ring-buffer layout, AddData timing, event queues, channel handling, and multi-port synchronization beyond the current waveOut path | Audio can crackle, be silent, drift, or play through incorrect channels. |
| **LV2 / system calls** | PARTIAL | Replace success-returning stubs with syscall-specific semantics for threads, events, semaphores, memory management, waits, and lifecycle operations | Guest synchronization and memory behaviour can fail even when a syscall appears to succeed. |
| **HLE library coverage** | PARTIAL | Expand NID coverage and real behaviour for cellGcmSys, cellSpurs, cellAudio, cellFs, libc, libpad, save-data APIs, and networking APIs; log unknown NIDs with useful context | Games invoke many imports that are currently null, stubbed, or only partially implemented. |
| **Save data** | PLANNED | Implement save-data enumeration, creation, metadata, read/write, and error behaviour with a safe per-title host mapping | Titles cannot reliably create, load, or preserve progress. |
| **Controller / libpad** | PARTIAL | Extend beyond XInput digital mapping to pressure-sensitive buttons, Sixaxis/motion where feasible, controller state details, and fuller libpad semantics | Some titles require analog pressure or richer controller state than digital buttons provide. |
| **Timing and threads** | PARTIAL | Introduce a more consistent PPU/SPU/RSX timing model, thread scheduling, waits, event delivery, and synchronization; avoid relying on cooperative progress alone | Races, incorrect speed, missed events, and softlocks can occur when guest timing assumptions are violated. |
| **PRX / modules** | PARTIAL | Strengthen module discovery, loading, memory placement, relocation, imports/exports, initialization, and unload lifecycle | Games that depend on dynamically loaded modules may fail even when the main ELF is lifted. |

### Suggested implementation order

1. **RSX/GCM draw pipeline**: move from clear/flip-only output toward surfaces, textures, shaders, draw state, and actual geometry.
2. **PPU correctness and diagnostics**: implement missing instruction semantics and replace silent recovery paths with traceable failures.
3. **SPU ISA and MFC/DMA**: expand instruction coverage and make DMA/tag/stall behaviour reliable.
4. **LV2, HLE, and PRX loading**: establish correct thread, synchronization, import, and module lifecycle behaviour.
5. **cellFs/VFS and save data**: make content lookup, directory operations, errors, and persistence reliable.
6. **SPURS and timing**: improve task execution, scheduling, and cross-processor synchronization.
7. **Audio and controller fidelity**: complete event/ring behaviour, richer pad state, and per-title validation.
8. **Compatibility validation**: test each change against reproducible homebrew tests and per-title logs; update game reports only with observed results.

Dependencies may change as testing reveals blockers. Each item should move from **PLANNED/PARTIAL** only after implementation and targeted validation; a stub or successful return code alone does not count as completion.

### Acceptance criteria for playability work

- Unsupported instructions, NIDs, syscalls, and graphics methods are visible in logs rather than silently treated as successful.
- Unit/smoke tests cover new instruction, DMA, filesystem, scheduling, and HLE behaviour where practical.
- Graphics progress is validated with actual draw calls and resources, not only clear/flip counts.
- Save/load, controller, audio, and thread behaviour are tested independently before marking a title-level improvement.
- Per-game status distinguishes static lift/build success from boot, rendered gameplay, input, audio, and save-data validation.
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
