# PPSX33 Roadmap

Legend: [x] implemented or verified, [~] partial, [ ] planned or not yet verified.

## Phase 1 - UI and project structure
- [x] WinForms window with Load ELF, Decompile, Build, and Copy to EBOOT folder actions
- [x] Project folder creation: `<app directory>/<GameName>/{input,codebase,output}`
- [x] Settings for graphics backend, CPU thread count, and RPCS3 path
- [x] RPCS3 launcher helper for the external ELF workflow
- [x] Progress marquee, Cancel, status line during Decompile/Build
- [x] In-app Lift report tab (loads `lift_report.txt`)
- [x] Find decrypted ELF (scan near EBOOT + RPCS3 cache paths, verify ELF64-BE magic)
- [x] Single CMake/VS project Rebuild All path (C++ core + GUI)
- Acceptance: load a valid ELF and complete the project-generation workflow. **Met.**

## Phase 2 - ELF decompile (PPU code to C++)
- [x] ELF64 big-endian loader, PT_LOAD segments, and OPD entry handling
- [x] Symbol table parse (when present), OPD table scan, PRX/module string heuristic
- [x] Embedded SPU ELF detection (ELF32 + EM_SPU) and dump as `spu_image_XX.bin`
- [x] Analysis report (`analysis_report.txt`) written next to lift report
- [x] PPU lifter: GOW3 snapshot 100% static translation (1,285,560 / 1,285,560)
- [~] Instruction semantics: many families real, long-tail approximate or nop
- [x] Atomics baseline: `lwarx/stwcx./ldarx/stdcx.` reservation fields
- [~] FPU and FPSCR (common ops; full FPSCR model still open)
- [~] VMX/AltiVec (loads/stores, logic, splat, bulk approx for remainder)
- [x] Function descriptor discovery via entry OPD + data-segment OPD scan
- [~] `.sceStub` / PRX: string+NID heuristic only (not full NID resolution table)
- [x] Detect and extract embedded SPU ELF images
- Acceptance: homebrew sample with correct runtime output. **Not met** (static coverage only).

## Phase 3 - PPU execution on x86-64
### 3a - CPU correctness
- [x] Baseline BE memory, CR, branches, LR/CTR, basic syscalls
- [x] Demand-paged guest memory (`ps3rt_touch`)
- [x] External PC: return-via-LR stub (cap 64 escapes)
- [ ] Full XER CA/OV/SO and FPSCR modeling
- [ ] Differential tests against RPCS3 interpreter

### 3b - OS and library compatibility
- [x] Exit and tty-write syscalls
- [~] Memory-management syscalls (simple heap pool)
- [ ] PPU threads, synchronization, timers
- [ ] Filesystem HLE
- [ ] Common PRX modules (sysutil, pad, audio, gcm, spurs, save, libc)

### 3c - Performance and threading
- [ ] Host-thread scheduling for PPU threads
- [ ] Function-level codegen, register caching
- [ ] Optional PGO/LTO
- Acceptance: multithreaded tests match expected output. **Not met.**

## Phase 4 - SPU execution
- [x] SPU context skeleton (LS store, mailbox, MFC DMA API)
- [x] LV2 SPU syscalls stubbed to CELL_OK
- [ ] Full SPU ISA lift from extracted images
- [ ] SPURS/task HLE and host-thread scheduling
- Acceptance: homebrew SPU test correct. **Not met.**

## Phase 5 - RSX graphics
- [x] Graphics backend selector API and GUI setting
- [ ] GCM / cellGcmSys HLE, NV47 state, shader translate, backends
- Acceptance: homebrew render tests. **Not met.**

## Phase 6 - Native output generation
- [x] MSVC path emits `game.exe` + `ps3rt.dll` + `guest_image.bin`
- [x] Post-build cleanup; reports retained at project root / output
- [~] Runtime validation: starts; external stub mitigates early leave-range
- [ ] Single-file embed; incremental build UX
- Acceptance: guest executes correctly without leaving range. **Not met.**

## Current GOW3 measurement

1,285,560 instruction instances, 1,285,560 translated, 0 unimplemented, 157 chunks (100% static translation for this snapshot). Does not prove semantic correctness or game compatibility.
