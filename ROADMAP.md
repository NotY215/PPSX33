# PPSX33 Roadmap

**Last updated:** 2026-10-15  
**Scope:** Tooling, packaging, runtime playability, and Phase XXI lettered work for the experimental PlayStation 3 static recompiler.

PPSX33 is a **research toolkit**, not a retail emulator or a finished port of any commercial game. Labels describe engineering work present in the tree. They do **not** claim verified boot, full graphics, audio fidelity, or playability for any title.

---

## How to read this document

| Label | Meaning |
|-------|---------|
| **DONE** | Code landed for this milestone |
| **TESTING** | Present and usable; regressions and incomplete semantics expected |
| **PARTIAL** | Core path exists; accuracy or coverage still incomplete |
| **PLANNED** | Described for future work; not a commitment |

---

## Track overview

```text
I – XIX        Tooling & core pipeline (TESTING)
XX-A – XX-G    Packaging & host UX (DONE)
XX-H – XX-O    Runtime playability base (DONE in tree)
XXI (A – Z)    Post-XX lettered implementations (DONE in tree)
```

**Phase XXI** is the collective name for all post–XX-O lettered work **A through Z**.

---

## Phases I – XIX — core pipeline (TESTING)

| Phase | Theme | Summary |
|-------|--------|---------|
| **I** | Project shell | Repository layout, build scripts, native API surface |
| **II** | ELF load | ELF64 big-endian loadable segments |
| **III** | OPD / symbols | Entry descriptors, symbol and OPD discovery |
| **IV** | Static analysis | PRX/NID heuristics, embedded SPU image detection |
| **V** | PPU lift | Chunk-based static translation to host C++ |
| **VI** | Coverage reports | Lift reports, unimplemented opcode tallies (static only) |
| **VII** | Runtime shell | `ps3rt.dll`, guest image map, boot stages |
| **VIII** | Memory model | Guest RAM, stack, basic alloc helpers |
| **IX** | HLE imports | NID table and import stubs |
| **X** | Syscall path | LV2 dispatch entry points |
| **XI** | Control recovery | Force-out / region assist when PC leaves translated chunks |
| **XII** | Pad input | XInput + keyboard map |
| **XIII** | Build driver | MSVC project generation and link of game-named EXE |
| **XIV** | Dwell assist | Sustained-PC hardware progress pokes (no thrash) |
| **XV** | RSX host present | D3D11 clear/flip path |
| **XVI** | Audio / save hooks | Early cellAudio and save dispatch stubs |
| **XVII** | Full pad map | Button/stick mapping and focus-gated log |
| **XVIII** | GUI workflow | WinForms load / decompile / build / run / copy |
| **XIX** | RSX depth / FIFO | Depth-stencil, method batching, coalesced Present |

---

## Phase XX-A … XX-G — packaging & host UX (DONE)

| ID | Status | Summary |
|----|--------|---------|
| **XX-A** | DONE | Game-named EXE + runtime binaries + pad_map.json |
| **XX-B** | DONE | cellFs path mapping + early PSARC TOC |
| **XX-C** | DONE | Copy output into PS3_GAME/USRDIR |
| **XX-D** | DONE | Controller mapping as pad_map.json |
| **XX-E** | DONE | Legal / TERMS first-run (system browser) |
| **XX-F** | DONE | Post-build layout |
| **XX-G** | DONE | Named run logs; focus-gated pad log |

---

## Phase XX-H … XX-O — runtime playability (DONE in tree)

| ID | Status | Summary |
|----|--------|---------|
| **XX-H** | DONE | Null-loop recovery; soft GCM flip/wait |
| **XX-I** | DONE | Fuller cellGcmSys HLE |
| **XX-J** | DONE | PSARC TOC + zlib streaming |
| **XX-K** | DONE | SPU ISA interpreter core |
| **XX-L** | DONE | MFC put/get, list DMA, tags |
| **XX-M** | DONE | SPURS-like job queue |
| **XX-N** | DONE | cellAudio → waveOut; GDI overlay |
| **XX-O** | DONE | ICON0 window/taskbar icon |

---

## Phase XXI — lettered implementations A – Z (DONE in tree)

Phase **XXI** groups every post–XX-O lettered milestone. Each letter remains a useful log/tag boundary; the phase name is **XXI**.

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **A** | RSX surfaces + blit | DONE | Color/depth surface fields, COLOR_OFFSET-style methods |
| **B** | PPU accuracy helpers | DONE | mul/div/rotate; LR-first recovery |
| **C** | SPU ISA expansion | DONE | Logic, compares, rotates, float |
| **D** | MFC / DMA fidelity | DONE | List layout, tag poll, queue depth |
| **E** | SPURS jobchain | DONE | ready, chain_next, completion cause |
| **F** | cellFs directories | DONE | opendir / readdir / closedir |
| **G** | Audio ring / notify | DONE | Ring indices, notify key |
| **H** | LV2 syscalls expand | DONE | Mutex, event, sem, thread, memory, fs |
| **I** | RSX draw / shader state | DONE | Vertex/fragment binds, program id |
| **J** | PPU lifter-facing helpers | DONE | Shared helpers for generated code |
| **K** | Audio multi-port | DONE | Up to 8 ports, per-port rings |
| **L** | RSX texture unit | DONE | TEX offset/format/width/height/pitch |
| **M** | Lifter helper surface | DONE | PS3_LIFTER_HELPERS markers |
| **N** | cellFs errno / SELF | DONE | CellFs-style errno; SELF/SCE reject |
| **O** | Library NID HLE | DONE | sysutil/sysmodule/pad/kb/mouse/spurs stubs |
| **P** | Save data lifecycle | DONE | Create/list/load/save/delete HLE |
| **Q** | Pad pressure / sixaxis | DONE | Pressure array; accel stubs; sensor toggle |
| **R** | Timing model | DONE | PPU/SPU phase ticks in progress_hw |
| **S** | PRX module registry | DONE | Load tracking on sysmodule / first NID |
| **T** | RSX textured draws | DONE | Log/tint when draws use bound texture |
| **U** | Deeper NID behaviour | DONE | GetSystemParamInt/String, CheckCallback |
| **V** | Host texture upload | DONE | Guest TEX_OFFSET → D3D11 dynamic texture |
| **W** | PRX relocate HLE | DONE | Pseudo base/entry; export bind table |
| **X** | Frame pacing | DONE | ~60 Hz host wait around flip assist |
| **Y** | Fullscreen texture blit | DONE | Scale uploaded guest tex to backbuffer; `[rsx] Y` |
| **Z** | PRX ELF load | DONE | ELF64 BE PT_LOAD into guest RAM; SELF skipped; `[prx] Z` |

---

## Symptom guide

| Symptom | Check in latest run log |
|---------|-------------------------|
| Solid blue / black | GCM; PSARC; `[rsx] Y` blit / `V` upload |
| SPU stall | SPU unhandled; MFC; SPURS |
| Missing assets | cellFs / PSARC; SELF rejected |
| PRX / module | `[prx] Z` ELF load or SELF skip |
| Timing / speed | `[time] X` |
| No window icon | Run from USRDIR; ICON0 |

---

## Recommended workflow

1. Accept legal notice (TERMS in system browser).  
2. Load a **decrypted** ELF.  
3. Decompile / lift.  
4. Set controller map before Build.  
5. Build → copy into `PS3_GAME/USRDIR`.  
6. Run the host EXE from USRDIR.  
7. On issues, attach the **newest** run log.

---

## Related docs

[README.md](README.md) · [USER_MANUAL.md](USER_MANUAL.md) · [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) · [LEGAL.md](LEGAL.md) / [TERMS.md](TERMS.md)

---

*PPSX33 does not include proprietary decryption keys or copyrighted game assets.*
