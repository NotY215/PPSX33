# PPSX33 Roadmap

Legend:

| Mark | Meaning |
| --- | --- |
| [x] | Implemented or verified in the current codebase |
| [~] | Partial, heuristic, or approximate |
| [ ] | Planned or not yet verified |

A checked item does **not** imply full semantic correctness or game compatibility unless the phase acceptance criteria say so.

**Priority order**

1. **God of War III (GOW3)** — primary title for runtime, HLE, and eventual playability work.
2. **Uncharted 2: Among Thieves** — secondary **opcode-expansion only** target (static decompile / lift reports). Do **not** build or launch `game.exe` for Uncharted 2 until GOW3 runtime goals are further along.

---

## Phase 1 — UI and project workflow

### 1A. Application shell

- [x] WinForms GUI (`PS3Recomp.exe`)
- [x] Tabs: Log, Lift report, Roadmap
- [x] Status line and marquee progress during long steps
- [x] Cancel control (cooperative; native steps may finish the current call)
- [x] Settings: graphics backend, PPU thread count, RPCS3 path

### 1B. Project workflow

- [x] Load decrypted ELF and create `<root>/<game>/{input,codebase,output}`
- [x] Decompile (lift) and Build actions via `ps3core`
- [x] Copy `game.exe` / `ps3rt.dll` / `guest_image.bin` next to EBOOT folder
- [x] Find decrypted ELF (near EBOOT + RPCS3 cache trees, ELF64-BE check)
- [x] RPCS3 launcher helper (decrypt workflow only)

### 1C. Build integration

- [x] CMake x64-Release style output under `build/`
- [x] Single-tree Rebuild All path (native core + GUI publish to `build/dist/`)
- [x] MSVC compiler path and `Compilers-files` staging docs
- [ ] Verify full GUI workflow on a clean Windows install
- [ ] Structured progress percentages from native lift/build (not only marquee)

**Acceptance:** Load a valid ELF; complete create → decompile → build → inspect output. **Core path met; clean-install verification still open.**

---

## Phase 2 — ELF analysis and PPU lifting

### 2A. ELF loader and analysis

- [x] ELF64 big-endian, PowerPC64, PT_LOAD segments
- [x] Entry OPD handling
- [x] Symbol tables when present
- [x] OPD / function-descriptor discovery heuristics
- [~] PRX / module string + NID heuristics (not full import resolver)
- [x] Embedded SPU ELF detection → `spu_image_XX.bin`
- [x] `lift_report.txt` and `analysis_report.txt`

### 2B. PPU instruction coverage (static)

- [x] GOW3 lift: 1,285,560 / 1,285,560 translated, 0 unimplemented, 157 chunks (100% static for that snapshot)
- [~] Integer ALU, logical, shifts/rotates, loads/stores, update forms
- [~] Branches, CR ops, LR/CTR, selected sync/trap
- [~] Atomics (`lwarx`/`stwcx.`/`ldarx`/`stdcx.`) reservation fields
- [~] FPU common ops; FPSCR incomplete
- [~] VMX/AltiVec selected ops + approximate long-tail
- [~] Long-tail encodings emitted as nop / soft-continue rather than hard halt

### 2C. Multi-game opcode expansion (Uncharted 2)

**Scope:** Decompile / lift only. **No** Uncharted 2 `game.exe` build or launch until GOW3 is the active runtime target.

- [ ] Obtain legally owned decrypted Uncharted 2 ELF
- [ ] Run lift; save `docs/games/Uncharted2/` report snapshot
- [ ] Diff unimplemented / approximate opcodes vs GOW3 baseline
- [ ] Implement high-frequency missing or weak opcodes in `ppu_lifter.cpp`
- [ ] Re-lift GOW3 and Uncharted 2; record before/after counts
- [ ] Add small regression ELF tests where practical

### 2D. Quality gates

- [ ] Differential tests vs a trusted PowerPC reference (e.g. RPCS3 interpreter) for hot opcodes
- [ ] Full PRX/NID import and export resolution
- [ ] Per-family regression matrix (integer, FP, VMX, atomic, branch)

**Acceptance:** Supported instructions pass differential tests; a homebrew sample produces correct output. **Static GOW3 100% does not satisfy this.**

---

## Phase 3 — PPU runtime (host execution)

**Primary title for this phase: GOW3.**

### 3A. CPU state and memory

- [x] BE memory helpers, GPR/FPR/VPR, CR, branches, LR/CTR
- [x] Demand-commit guest memory (`ps3rt_touch`) on Windows
- [x] Stack and simple heap pool regions
- [~] External PC left range → return-via-LR stub (bounded escapes)
- [ ] Full XER CA/OV/SO
- [ ] Full FPSCR model
- [ ] Fix GOW3 early halt (`pc=0x39800000`, TOC/r2 corruption investigation)
- [ ] Differential tests for branches, memory, side effects

### 3B. OS and library HLE

- [x] Process exit and tty-write syscalls
- [~] Simple `sys_memory_allocate`-style heap
- [~] Selected SPU-related LV2 stubs return CELL_OK
- [ ] PPU threads, sync primitives, timers
- [ ] Filesystem and save-data HLE
- [ ] Common PRX modules: cellSysutil, cellPad, cellAudio, cellGcmSys, cellSpurs, cellSaveData, libc-compat

### 3C. Performance and threading

- [ ] Host scheduling honoring configured PPU thread count
- [ ] Function-level codegen improvements and register caching
- [ ] Optional PGO / LTO

**Acceptance:** Runtime tests match expected output; guest does not leave translated range unexpectedly. **Not met (GOW3 still early-halts).**

---

## Phase 4 — SPU execution

### 4A. Infrastructure

- [x] SPU context + 256 KB local store (multiple instances)
- [x] API stubs: create/destroy/load/run/stop, mailbox, MFC DMA
- [x] Extract embedded SPU images during PPU lift (Phase 2)

### 4B. ISA and scheduling

- [ ] Full SPU instruction interpreter or static lifter
- [ ] Correct MFC DMA, mailboxes, signals, atomics
- [ ] SPU thread groups and SPURS/task HLE
- [ ] Host-thread scheduling for SPU work

**Acceptance:** Homebrew SPU test produces correct output. **Not met** (stub run loop only).

---

## Phase 5 — RSX graphics

### 5A. Control path

- [x] GUI backend selector (D3D10 / D3D11 / Vulkan)
- [ ] GCM command-buffer parse and cellGcmSys HLE
- [ ] Display / flip queue

### 5B. Device state and shaders

- [ ] NV47 graphics state tracking
- [ ] RSX vertex/fragment microcode → HLSL / SPIR-V
- [ ] Textures, swizzling, depth, render targets

### 5C. Host backends

- [ ] Direct3D 11 backend
- [ ] Vulkan backend
- [ ] Direct3D 10 backend (optional)

**Acceptance:** Homebrew render tests correct on each supported backend. **Not met.**

---

## Phase 6 — Native output and integration

### 6A. Artifacts

- [x] `ps3core` C API + CLI
- [x] Build driver invokes MSVC (or g++ fallback)
- [x] Emits `game.exe`, `ps3rt.dll`, `guest_image.bin`
- [x] Post-build cleanup; retain reports at project root / output

### 6B. Packaging and CI

- [~] Runtime startup experimental; external-PC stub in place
- [ ] Optional single-file package (embed image + static runtime)
- [ ] Incremental builds and richer compile progress
- [ ] CI on supported host platforms

**Acceptance:** Synthetic and homebrew guests execute correctly; then evaluate per commercial title. **Not met for commercial titles.**

---

## Game coverage matrix

| Title | Role | Static lift | Build/run | Notes |
| --- | --- | --- | --- | --- |
| God of War III | **Primary** | 100% snapshot (1,285,560 insn) | Yes (experimental) | Runtime early PC leave; focus of Phase 3 |
| Uncharted 2: Among Thieves | **Opcode expansion only** | Not measured yet | **No** (deferred) | Lift reports only; feed Phase 2C |
| Synthetic / homebrew ELF | Regression | Smoke scripts | Yes when available | Prefer over commercial titles for correctness tests |

### God of War III

See [docs/games/GOW3/GOW3.md](docs/games/GOW3/GOW3.md).

### Uncharted 2: Among Thieves

See [docs/games/Uncharted2/UNCHARTED2.md](docs/games/Uncharted2/UNCHARTED2.md).

Workflow when starting Uncharted 2 work:

1. Load decrypted ELF in the GUI (or CLI lift only).
2. Decompile; do **not** run Build for this title yet.
3. Archive `lift_report.txt` / `analysis_report.txt` under `docs/games/Uncharted2/`.
4. Implement missing opcodes from the TODO list (highest frequency first).
5. Re-run GOW3 lift to ensure no regressions in static counts.
6. Only after GOW3 runtime is healthier, reconsider Uncharted 2 native build.

---

## Suggested near-term sequence

| Step | Phase | Action |
| --- | --- | --- |
| 1 | 3A | Diagnose GOW3 `pc=0x39800000` (code at `lr≈0x103ac`, TOC/r2) |
| 2 | 2C | Lift Uncharted 2 ELF; land opcode gaps in the lifter |
| 3 | 2B/2D | Harden approximate ops on hot paths; add tests |
| 4 | 3B | Expand LV2 / PRX HLE needed by GOW3 CRT startup |
| 5 | 4B | Real SPU ISA for extracted images |
| 6 | 5 | RSX when CPU path reaches a flip |

---

## Related docs

- [PPU coverage](docs/PPU_COVERAGE.md)
- [Game coverage index](docs/games/README.md)
- [README](README.md)
