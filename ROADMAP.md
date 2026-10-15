# PPSX33 Roadmap

**Last updated:** 2026-10-15  
**Scope:** Tooling, packaging, runtime playability, and post–XX lettered work for the experimental PlayStation 3 static recompiler.

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
I – XIX     Tooling & core pipeline (TESTING)
XX-A – XX-G Packaging & host UX (DONE)
XX-H – XX-O Runtime playability base (DONE in tree)
A – K       Post-XX lettered implementations (DONE in tree)
Remaining   Shaders depth, deeper HLE, saves, PRX, timing, pad
```

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

These phases remain **testing** quality. Instruction counts are static-lift snapshots, not playability metrics.

---

## Phase XX-A … XX-G — packaging & host UX (DONE)

| ID | Status | Summary |
|----|--------|---------|
| **XX-A** | DONE | Game-named EXE + `ps3rt.dll` + `guest_image.bin` + `pad_map.json` |
| **XX-B** | DONE | cellFs path mapping + early PSARC TOC |
| **XX-C** | DONE | Copy output into `PS3_GAME/USRDIR` (auto-find `EBOOT.BIN`) |
| **XX-D** | DONE | Controller mapping persisted as `pad_map.json` |
| **XX-E** | DONE | Legal / TERMS first-run; links open in the system browser |
| **XX-F** | DONE | Post-build layout: reports at project root; runtime under `output/` |
| **XX-G** | DONE | Named run logs; pad log only while the game window is focused |

---

## Phase XX-H … XX-O — runtime playability (DONE in tree)

| ID | Status | Summary |
|----|--------|---------|
| **XX-H** | DONE | Null-loop recovery; soft GCM flip/wait |
| **XX-I** | DONE | Fuller `cellGcmSys` HLE (config, display buffers, flip lifecycle) |
| **XX-J** | DONE | PSARC TOC + zlib block streaming |
| **XX-K** | DONE | SPU ISA interpreter (load/store, arith, branch, channels, stop) |
| **XX-L** | DONE | MFC put/get, list DMA, barriers, tags |
| **XX-M** | DONE | SPURS-like job queue + `cellSpurs` HLE (8 SPU slots) |
| **XX-N** | DONE | `cellAudio` → waveOut; GDI status overlay; window title |
| **XX-O** | DONE | `ICON0.PNG` as window/taskbar icon when run from USRDIR |

---

## Post-XX lettered implementations

Alphabet continues after **XX-O**. Each letter is one focused implementation landed in-tree.

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **A** | RSX surfaces + blit | DONE | Color/depth surface fields, COLOR_OFFSET-style methods, present tagging |
| **B** | PPU accuracy helpers | DONE | mul/div/rotate helpers; recovery prefers LR before region skip |
| **C** | SPU ISA expansion | DONE | Additional logic, compares, rotates, single-float ops |
| **D** | MFC / DMA fidelity | DONE | List element layout, tag status = completed ∩ mask, command queue depth |
| **E** | SPURS jobchain | DONE | Job ready flag, chain successor, completion cause |
| **F** | cellFs directories | DONE | opendir / readdir / closedir against host folders |
| **G** | Audio ring / notify | DONE | Ring indices, notify event key HLE |
| **H** | LV2 syscalls expand | DONE | Mutex, event queue, semaphore, PPU thread, memory, fs syscall numbers |
| **I** | RSX draw / shader state | DONE | Vertex/fragment bind counts, shader program id, vertex array mask, expanded method decode |
| **J** | PPU lifter-facing helpers | DONE | Shared mul/div/rlwinm helpers for generated code; LR-first recover_return_pc |
| **K** | Audio multi-port | DONE | Up to 8 ports, per-port open flags and ring write counters |

### Remaining gaps

| Sno. | Name | Status | Work |
|------|------|--------|------|
| **L** | RSX raster / textures | PARTIAL | Textured draws, depth test on geometry, multi-RT |
| **M** | Lifter emission of helpers | PARTIAL | Generate calls to J helpers from the static lifter |
| **N** | cellFs error / SELF | PARTIAL | Richer errno mapping; explicit unsupported encrypted paths |
| **O** | Libraries / NID HLE | PARTIAL | Trace-driven expansion beyond current slices |
| **P** | Save data lifecycle | PLANNED | Create / enumerate / load / update with host save root |
| **Q** | Pad / Sixaxis | PARTIAL | Pressure and motion fields |
| **R** | Timing model | PARTIAL | Coordinated PPU / SPU / RSX timing |
| **S** | PRX modules | PARTIAL | Load, relocate, import/export |

---

## Symptom guide

| Symptom | What to check in the latest run log |
|---------|-------------------------------------|
| Solid blue / black | GCM activity; PSARC opens; `[rsx] I` shader/draw counts |
| SPU stall | Unhandled SPU ops; MFC list/tag lines; SPURS schedule |
| Missing assets | Directory ops; PSARC virtual open |
| Silent audio | `[audio] K` init / AddData / port open |
| No window icon | Run from USRDIR; ICON0 path resolution |

---

## Recommended workflow

1. Accept legal notice (TERMS in system browser).  
2. Load a decrypted ELF (or decrypt EBOOT via a separately installed tool).  
3. Decompile / lift.  
4. Set controller map before Build.  
5. Build → copy into `PS3_GAME/USRDIR`.  
6. Run the host EXE from USRDIR.  
7. On issues, attach the **newest** run log.

---

## Related docs

| Doc | Purpose |
|-----|---------|
| [README.md](README.md) | Project status and lettered summary |
| [USER_MANUAL.md](USER_MANUAL.md) | End-user workflow and troubleshooting |
| [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) | Architecture for contributors |
| [docs/PPU_COVERAGE.md](docs/PPU_COVERAGE.md) | Static lift coverage notes |
| [LEGAL.md](LEGAL.md) / [TERMS.md](TERMS.md) | Legal notices |

---

*PPSX33 does not include, use, or distribute proprietary decryption keys or copyrighted game assets. You must supply your own legally obtained dumps and toolchains.*
