# PPSX33 Roadmap

Legend: [x] implemented in the current codebase, [~] partial or heuristic, [ ] not implemented or not verified. A checked implementation item does not imply semantic correctness or game compatibility.

## Phase 1: UI and project workflow

- [x] WinForms GUI with Load ELF, Find decrypted ELF, Decompile, Build, Copy to EBOOT folder, Cancel, status, log, and lift-report tabs
- [x] Project layout: `<root>/<game>/{input,codebase,output}`
- [x] Settings for graphics backend and PPU thread count
- [x] RPCS3 launch helper and scan of nearby EBOOT/RPCS3 cache paths for ELF64 big-endian input
- [x] CMake/Ninja native build and optional .NET GUI publish
- [ ] Verify the full GUI workflow on clean Windows installations
- [ ] Reliable cancellation of long-running native work and structured progress reporting

Acceptance: load a valid ELF and complete project creation, lifting, building, and output inspection.

## Phase 2: ELF analysis and PPU lifting

- [x] ELF64 big-endian loader for loadable segments and entry point handling
- [x] Symbol parsing when symbol tables are present
- [x] OPD/function descriptor discovery heuristics
- [~] PRX import/module detection using strings and NID heuristics; this is not a complete import resolver
- [x] Embedded SPU ELF detection and extraction to `spu_image_XX.bin`
- [x] `lift_report.txt` and `analysis_report.txt` generation
- [x] GOW3 lift snapshot reports 1,285,560 translated instruction instances, zero unimplemented, across 157 chunks
- [~] Instruction semantics remain uneven; some long-tail operations are approximate or emitted as no-ops
- [~] Atomic instruction reservation handling exists, but full architectural behavior requires validation
- [~] Common FPU/FPSCR operations are present; full FPSCR behavior is not complete
- [~] VMX/AltiVec support includes selected operations and approximations for other decoded forms
- [ ] Differentially validate instruction behavior against a trusted PowerPC implementation
- [ ] Complete PRX/NID import and export resolution
- [ ] Expand regression coverage for every supported instruction family

Acceptance: supported instructions pass differential tests, and a homebrew sample produces the expected output. Static translation counts alone do not satisfy this criterion.

## Phase 3: PPU runtime correctness

### 3A. CPU state and memory

- [x] Big-endian guest memory helpers, GPR/FPR/vector register storage, CR comparisons, branch helpers, and LR/CTR state
- [x] Demand-commit guest-memory helper on Windows
- [~] Guest PC escape handling can return through LR with a bounded retry path
- [ ] Full XER CA/OV/SO and FPSCR modeling
- [ ] Differential tests for branch conditions, exceptions, memory ordering, and register side effects

### 3B. OS and library compatibility

- [x] Basic exit and tty-write syscall handling
- [~] Simple guest heap pool and selected memory-management behavior
- [ ] PPU thread scheduling, synchronization primitives, and timers
- [ ] Filesystem and save-data HLE
- [ ] Common PRX modules including sysutil, pad, audio, GCM, SPURS, save-data, and libc compatibility

### 3C. Performance

- [ ] Host scheduling that honors configured PPU thread count
- [ ] Function-level code generation, register caching, and profile-guided optimization

Acceptance: runtime tests match expected output and do not leave the translated guest-code range unexpectedly.

## Phase 4: SPU

- [x] SPU context and local-store structures
- [x] Stub APIs for SPU create/destroy/load/run/stop, mailboxes, signals, and MFC DMA
- [ ] Full SPU instruction-set interpreter or lifter
- [ ] Correct SPU scheduling, DMA semantics, event behavior, and SPURS/task support

The existing SPU run path is a stub, not functional SPU execution.

## Phase 5: RSX graphics

- [x] GUI setting for D3D10, D3D11, and Vulkan backend selection
- [ ] GCM command-buffer and cellGcmSys handling
- [ ] NV47 graphics state tracking
- [ ] RSX vertex/fragment program translation and shader execution
- [ ] Texture formats, swizzling, depth buffers, and render targets
- [ ] Working Direct3D/Vulkan backend implementation

A backend selector is not evidence that those graphics backends render PS3 output.

## Phase 6: Native output and integration

- [x] C API shared library and CLI frontend
- [x] Build driver generates build files and invokes host compiler tools
- [x] Windows output path can emit `game.exe`, `ps3rt.dll`, and `guest_image.bin`
- [~] Runtime startup and guest-PC escape handling remain experimental
- [ ] Stable single-file output and incremental builds
- [ ] CI validation across supported host platforms

Acceptance: synthetic and homebrew guests execute correctly, then compatibility can be evaluated per title.

## Game coverage

### God of War III

The latest supplied snapshot reports 1,285,560 instruction instances translated, zero unimplemented, and 157 chunks. This is 100% static translation coverage for that specific lift report, not proof that all instruction semantics are correct or that GOW3 runs correctly. See [GOW3 record](docs/games/GOW3/GOW3.md).

### Uncharted 2: Among Thieves

Planned as a future PPU coverage target. Once analysis begins, its lift report will help identify missing or incomplete opcode implementations and guide regression tests. No counts or coverage percentage are available yet. See the [Uncharted 2 analysis plan](docs/games/Uncharted2/UNCHARTED2.md).

Use additional games and small purpose-built ELF tests to discover instructions and runtime dependencies not present in existing snapshots. Keep PPU instruction coverage, SPU execution, RSX graphics, OS/library support, and full-game compatibility as separate metrics.
