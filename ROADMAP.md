# PPSX33 Roadmap

## Status legend

| Mark | Meaning |
| --- | --- |
| [x] | Present in the codebase or completed as explicitly measured |
| [~] | Partial, heuristic, approximate, or not fully validated |
| [ ] | Not implemented or not verified |

Implementation status does not establish semantic correctness or compatibility. Acceptance criteria define the evidence required to mark work complete.

## Project priorities

1. Improve PPU semantic correctness and runtime stability using synthetic tests and the God of War III (GOW3) and Uncharted 2 static-lift snapshots.
2. Inspect the Uncharted 2 reports for approximate or fallback translations, then prioritize semantic tests and fixes by frequency and correctness risk.
3. Develop SPU execution, RSX/GCM support, and system-library compatibility as separate workstreams.

## Phase 1: UI and project workflow

### Application shell
- [x] WinForms GUI (`PS3Recomp.exe`)
- [x] Log, lift report, and roadmap views
- [x] Status display and progress marquee
- [x] Cooperative cancellation control
- [x] Settings for graphics backend, PPU thread count, and RPCS3 path

### Project workflow
- [x] Load decrypted ELF and create `<root>/<game>/{input,codebase,output}`
- [x] Decompile and Build actions through `ps3core`
- [x] Copy output artifacts beside an EBOOT folder
- [x] Search nearby EBOOT and RPCS3 cache trees for ELF64-BE input
- [x] RPCS3 launcher helper for the external decrypt workflow

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
- [x] GOW3 snapshot: 1,285,560 translated instances, zero reported unimplemented instances, 157 chunks
- [x] Uncharted 2 snapshot: 3,485,016 translated instances, zero reported unimplemented instances, 426 chunks
- [~] Integer arithmetic, logical operations, shifts/rotates, loads/stores, and update forms
- [~] Branches, condition-register operations, LR/CTR, selected synchronization and trap instructions
- [~] Atomic reservation handling
- [~] Common floating-point operations; FPSCR behavior remains incomplete
- [~] Selected VMX/AltiVec operations; some long-tail handling remains approximate
- [ ] Differential semantic tests against a trusted PowerPC reference
- [ ] Complete PRX import/export and NID resolution
- [ ] Regression coverage for each supported instruction family

### Multi-game analysis: Uncharted 2
- [x] Analyze a decrypted ELF and complete the initial static lift
- [x] Record console metrics in the game coverage documentation
- [ ] Review the generated lift and analysis reports for approximate, fallback, or semantically risky translations
- [ ] Archive suitable text reports under `docs/games/Uncharted2/` after reviewing them for sensitive paths and proprietary content
- [ ] Identify instruction families that need semantic validation or regression coverage
- [ ] Add isolated synthetic regression tests where practical
- [ ] Re-run GOW3 and synthetic tests after lifter changes

**Acceptance:** Implemented instructions pass differential tests where a reference is available, and synthetic or homebrew programs produce expected results. Static translation counts alone are insufficient.

## Phase 3: PPU runtime correctness

### CPU state and memory
- [x] Big-endian memory helpers, GPR/FPR/VPR storage, CR helpers, branches, LR/CTR
- [x] Windows demand-commit guest-memory helper
- [x] Stack and simple heap-pool regions
- [~] Bounded handling for guest PC leaving translated code
- [ ] Full XER CA/OV/SO behavior
- [ ] Complete FPSCR model
- [ ] Differential tests for branch conditions, exceptions, memory ordering, and register side effects
- [ ] Resolve GOW3 early-halt and TOC/r2 investigation using captured diagnostics

### OS and library compatibility
- [x] Process exit and tty-write syscall handling
- [~] Simple guest heap and selected memory-management behavior
- [~] Selected SPU-related LV2 stubs
- [ ] PPU scheduling, synchronization primitives, and timers
- [ ] Filesystem and save-data HLE
- [ ] Common PRX modules for sysutil, pad, audio, GCM, SPURS, save-data, and libc compatibility

### Performance
- [ ] Host scheduling that honors configured PPU thread count
- [ ] Function-level code generation and register caching
- [ ] Optional profile-guided optimization and link-time optimization

**Acceptance:** Runtime tests match expected output and guest execution remains within valid translated control flow. Commercial-game compatibility is evaluated separately.

## Phase 4: SPU execution

- [x] SPU context and 256 KB local-store structures
- [x] API stubs for create/destroy/load/run/stop, mailbox, and MFC DMA
- [x] Embedded SPU image extraction during PPU lifting
- [ ] Full SPU instruction interpreter or lifter
- [ ] Correct MFC DMA, mailbox, signal, and atomic semantics
- [ ] SPU thread groups and SPURS/task support
- [ ] Host scheduling for SPU work

**Acceptance:** A focused homebrew SPU test produces the expected output.

## Phase 5: RSX graphics

- [x] GUI selector for D3D10, D3D11, and Vulkan
- [ ] GCM command-buffer parsing and cellGcmSys HLE
- [ ] Display and flip queue
- [ ] NV47 graphics state tracking
- [ ] RSX vertex/fragment microcode translation
- [ ] Texture formats, swizzling, depth buffers, and render targets
- [ ] Working and tested host graphics backends

**Acceptance:** A homebrew rendering test produces correct output on each backend declared supported.

## Phase 6: Native output and integration

- [x] C API shared library and CLI frontend
- [x] Build driver invokes host compiler tools
- [x] Windows output can emit `game.exe`, `ps3rt.dll`, and `guest_image.bin`
- [~] Runtime startup and guest-PC escape handling remain experimental
- [ ] Optional single-file output
- [ ] Incremental builds and structured compile progress
- [ ] CI validation on supported host platforms

**Acceptance:** Synthetic and homebrew guests execute correctly before broader compatibility claims are made.

## Game coverage matrix

| Title | Role | Static lift | Runtime status |
| --- | --- | --- | --- |
| God of War III | Runtime-analysis baseline | 100% reported in one snapshot (1,285,560 instances) | Experimental; correct full-game execution not established |
| Uncharted 2: Among Thieves | Multi-game static-lift and semantic-analysis target | 100% reported in one snapshot (3,485,016 instances) | Not validated by these static-lift results |
| Synthetic/homebrew ELF | Regression tests | Measured per test | Expected output required for passing tests |

## Related documentation

- [Developer guide](docs/DEVELOPER_GUIDE.md)
- [PPU coverage](docs/PPU_COVERAGE.md)
- [Game coverage index](docs/games/README.md)
- [Project overview](README.md)
