# PPSX33 Roadmap

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="140" /></p>

## Status legend

| Mark | Meaning |
| --- | --- |
| [x] | Present in the codebase or completed as explicitly measured |
| [~] | Partial, heuristic, approximate, or not fully validated |
| [ ] | Not implemented or not verified |

Implementation status does not establish semantic correctness or compatibility.

## Project priorities

1. **GOW3** runtime stability (Phase 3) and semantic hardening of approximate PPU ops.
2. Use **Minecraft (PS3)**, **Uncharted 2**, and **Demon's Souls** static lifts to stress-test SPU interpreter and long-tail PPU paths (lift only; no native build yet for these titles).
3. SPU, RSX, and PRX HLE as separate tracks.

## Phase 1: UI and project workflow

### Application shell
- [x] WinForms GUI (`PS3Recomp.exe`)
- [x] Log, lift report, and roadmap views
- [x] Status display and progress marquee
- [x] Cooperative cancellation control
- [x] Settings for graphics backend, PPU thread count, and RPCS3 path
- [x] **Choose rpcs3.exe** on decrypt (and OPTIONS browse); path saved in settings.json

### Project workflow
- [x] Load decrypted ELF and create `<root>/<game>/{input,codebase,output}`
- [x] Decompile and Build actions through `ps3core`
- [x] Copy output artifacts beside an EBOOT folder
- [x] Search nearby EBOOT and RPCS3 cache trees for ELF64-BE input
- [x] **Decrypt EBOOT (RPCS3):** pick `rpcs3.exe` if needed, then `EBOOT.BIN`, runs `rpcs3.exe --decrypt`
- [x] Does **not** boot the game in RPCS3

### Build integration
- [x] CMake build output under `build/`
- [x] Native core and GUI publish path to `build/dist/`
- [x] MSVC discovery and compiler staging documentation
- [ ] Verify the full GUI workflow on a clean Windows installation
- [ ] Replace marquee-only progress with structured native progress

**Acceptance:** A valid ELF completes project creation, lifting, building, and artifact inspection on a clean supported environment.

## Phase 2: ELF analysis and PPU lifting

### ELF loader and analysis
- [x] ELF64 big-endian PowerPC64 and PT_LOAD handling
- [x] Entry OPD handling
- [x] Symbol tables when present
- [x] OPD/function-descriptor discovery heuristics
- [~] PRX/module string and NID heuristics
- [x] Embedded SPU ELF detection and extraction to `spu_image_XX.bin`
- [x] `lift_report.txt` and `analysis_report.txt` generation

### PPU instruction coverage
- [x] GOW3 snapshot: 1,285,560 translated, 0 unimplemented, 157 chunks
- [x] Minecraft (PS3) snapshot: 3,034,530 translated, 0 unimplemented, 371 chunks
- [x] Uncharted 2 snapshot: 3,485,016 translated, 0 unimplemented, 426 chunks
- [x] Demon's Souls snapshot: 6,343,506 translated, 0 unimplemented, 775 chunks
- [~] Integer, logical, shifts, loads/stores, branches, atomics, FPU, VMX (many approximate)
- [ ] Differential semantic tests against a trusted PowerPC reference
- [~] PRX import HLE + real NID table (GOW3 modules); expand on runtime hits

### Multi-game analysis
- [x] Minecraft (PS3): initial static lift and ELF analysis recorded
- [x] Uncharted 2: initial static lift and ELF analysis recorded
- [x] Demon's Souls: initial static lift and ELF analysis recorded
- [ ] Review approximate/fallback translations; add regression tests
- [ ] Re-run GOW3 after lifter changes

**Acceptance:** Differential tests + synthetic/homebrew correct output. Static counts alone are insufficient.

## Phase 3: PPU runtime correctness (GOW3 primary)

### CPU state and memory
- [x] BE memory, GPR/FPR/VPR, CR, branches, LR/CTR
- [x] Windows demand-commit guest memory
- [x] Stack and simple heap pool
- [~] Bounded external-PC return-via-LR
- [ ] Full XER / FPSCR
- [ ] Resolve GOW3 early halt (`pc=0x39800000`, TOC/r2)

### OS and library compatibility
- [x] Exit and tty-write
- [~] Simple heap / memory allocate
- [~] SPU-related LV2 stubs
- [~] Threads, sync, timers, filesystem, common PRX modules (partial HLE)

### Performance
- [ ] Host PPU thread scheduling, function-level codegen, optional PGO/LTO

**Acceptance:** Runtime tests match expected output; guest stays in translated control flow.

## Phase 4: SPU execution

- [x] SPU context: 128 x 128-bit GPRs, 256 KB local store (up to 8 instances)
- [x] API: create/destroy/load/run/stop, mailbox, MFC DMA
- [x] Embedded SPU image extraction during PPU lifting
- [~] Interpreter covering major ISA families
- [~] Full vector lane semantics for every instruction
- [~] MFC DMA get/put via channels and API
- [ ] Complete remaining SPU opcodes (FP, full shuffles, all channel numbers)
- [ ] SPURS/task HLE and multi-SPU host scheduling

**Acceptance:** Focused homebrew SPU test produces expected output.

## Phase 5: RSX graphics

- [x] GUI backend selector
- [~] GCM FIFO + host present (D3D11/D3D10; Vulkan module + GDI fallback); shaders/textures open

**Acceptance:** Homebrew render test on each supported backend.

## Phase 6: Native output and integration

- [x] C API, CLI, MSVC `game.exe` + `ps3rt.dll` + `guest_image.bin`
- [~] Runtime startup experimental
- [ ] Single-file package, incremental builds, CI

## Game coverage matrix

| Game | Role | Static lift | Runtime |
| --- | --- | --- | --- |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/God_of_War_Logo.png" width="78" alt="God of War logo" /> God of War III | Primary runtime | 1,285,560 / 100% snapshot | Experimental early halt |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Minecraft_Logo.png" width="78" alt="Minecraft logo" /> Minecraft (PS3) | Opcode / SPU stress (lift only) | 3,034,530 / 100% snapshot; 11 SPU images | Not targeted |
| <img src="https://www.pngkey.com/png/detail/983-9839044_uncharted-2-among-thieves.png" width="48" alt="Uncharted 2 logo artwork" /> Uncharted 2 | Opcode / SPU stress (lift only) | 3,485,016 / 100% snapshot; 7 SPU images | Not targeted |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Demon%27s_Souls_logo_black.svg" width="78" alt="Demon's Souls logo" /> Demon's Souls | Opcode / SPU stress (lift only) | 6,343,506 / 100% snapshot; 12 SPU images | Not targeted |
| Synthetic/homebrew | Regression | Per test | Required for pass |

## Related documentation

- [PPU coverage](docs/PPU_COVERAGE.md)
- [Game coverage index](docs/games/README.md)
- [Project overview](README.md)
