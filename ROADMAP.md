# PPSX33 Roadmap

Legend: [x] implemented or verified, [~] partial, [ ] planned or not yet verified. Completion requires the acceptance criteria for the phase to pass.

## Phase 1 - UI and project structure
- [x] WinForms window with Load ELF, Decompile, Build, and Copy to EBOOT folder actions
- [x] Project folder creation: `<app directory>/<GameName>/{input,codebase,output}`
- [x] Settings for graphics backend, CPU thread count, and RPCS3 path
- [x] RPCS3 launcher helper for the external ELF workflow
- [ ] Build and validate the GUI on Windows using the supported .NET targeting pack
- [ ] Progress reporting, cancellation, and in-app `lift_report.txt` display
- [ ] Automatic discovery of the decrypted ELF produced by RPCS3
- Acceptance: load a valid ELF and complete the project-generation workflow.

## Phase 2 - ELF decompile (PPU code to C++)
- [x] ELF64 big-endian loader, PT_LOAD segments, and OPD entry handling
- [~] PPU decoder and lifter; supported families are tracked in [PPU coverage](docs/PPU_COVERAGE.md)
- [ ] Remaining integer ISA families, including division/multiply variants, shifts, sign extension, indexed loads/stores, update forms, arithmetic carry, logical variants, condition-register operations, and synchronization instructions
- [ ] Atomic instructions: `lwarx/stwcx./ldarx/stdcx.`
- [ ] Expanded FPU and FPSCR behavior
- [ ] VMX/AltiVec vector-register support
- [ ] Function discovery using symbols, `.opd`, and call graph
- [ ] Parse `.sceStub` and PRX import/export tables, including NID resolution
- [ ] Detect and extract embedded SPU ELF images
- Acceptance: a real homebrew sample has no unsupported instructions in its tested execution path and produces correct output.

## Phase 3 - PPU execution on x86-64
### 3a - CPU correctness
- [x] Baseline big-endian memory helpers, condition-register comparisons, branch conditions, LR/CTR, and basic syscalls
- [ ] Full condition-register SO bit, XER CA/OV/SO, and FPSCR modeling
- [ ] Differential tests against RPCS3's interpreter for small test ELFs

### 3b - OS and library compatibility
- [~] Basic exit and tty-write syscalls
- [ ] Memory-management syscalls
- [ ] PPU thread creation, synchronization primitives, and timers
- [ ] Filesystem HLE for game data directories
- [ ] Common PRX modules, including cellSysutil, cellPad, cellAudio, cellGcmSys, cellSpurs, cellSaveData, and libc compatibility

### 3c - Performance and threading
- [ ] Host-thread scheduling for PPU threads, respecting the configured thread count
- [ ] Function-level code generation, direct calls, register caching, and optimized builds
- [ ] Optional profile-guided optimization and link-time optimization
- Acceptance: multithreaded tests use configured threads and match expected output.

## Phase 4 - SPU execution
- [ ] SPU ELF/`.sputext` extraction and instruction lifting
- [ ] MFC DMA, mailboxes, signals, and atomic operations
- [ ] SPU thread groups, SPURS/task libraries, and associated HLE
- [ ] Host-thread scheduling for SPU workloads
- Acceptance: a homebrew SPU test produces correct output.

## Phase 5 - RSX graphics
- [x] Graphics backend selector API and GUI setting
- [ ] GCM command-buffer parsing, cellGcmSys HLE, and display/flip queue
- [ ] NV47 graphics state tracking
- [ ] RSX vertex/fragment microcode translation to HLSL and SPIR-V
- [ ] Texture formats, swizzling, depth buffers, and render targets
- [ ] Direct3D 11, Vulkan, and Direct3D 10 backend implementations
- Acceptance: homebrew rendering tests produce correct output on each supported backend.

## Phase 6 - Native output generation
- [x] Build driver generates build files and invokes the selected compiler path
- [x] Output generation for `game.exe`, `ps3rt.dll`, and `guest_image.bin`
- [x] MSVC compilation/link path emits `game.exe` and `ps3rt.dll`
- [~] Runtime validation: execution starts, then the guest PC leaves the recompiled range
- [ ] Optional C#-generated front-end code
- [ ] Optional single-file output with embedded guest image and static runtime
- [ ] Incremental builds and parallel compile progress
- [ ] Startup validation of required compiler tools
- Acceptance: a test guest executes correctly without leaving the recompiled range and required runtime dependencies are resolved.

## Current GOW3 measurement

The latest reported lift contains 1,285,560 instruction instances: 1,285,560 translated and zero unimplemented across 157 chunks, giving 100% static translation coverage for this snapshot. This does not establish correct instruction semantics or game compatibility. See [GOW3 coverage record](docs/games/GOW3/GOW3.md).
