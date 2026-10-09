# Roadmap

Legend: [x] done, [~] partial, [ ] todo. Each phase lists **acceptance criteria**: a phase is done when they pass.

## Phase 1 - UI and project structure
- [x] WinForms window: Load ELF, Decompile, Build, Copy to EBOOT folder (`src/gui`)
- [x] Ask for **game name** and create `<app dir>/<GameName>/{input,codebase,output}` (`ps3_create_project`)
- [x] Settings: graphics backend (D3D10/D3D11/Vulkan), CPU thread count, RPCS3 path (`settings.json`)
- [x] RPCS3 launcher helper (decryption only, no gameplay)
- [ ] Compile and test the GUI on Windows with .NET 8 (never compiled yet)
- [ ] Progress bar and cancel button; show `lift_report.txt` in the UI
- [ ] Automatically find the decrypted ELF produced via RPCS3
- Acceptance: user can go from an ELF file to a populated project folder with one click each step.

## Phase 2 - ELF decompile (PPU code -> C++)
- [x] ELF64 big-endian loader, PT_LOAD segments, OPD entry (`elf_loader.cpp`)
- [~] PPU decoder/lifter, ~40 instructions (`docs/PPU_COVERAGE.md`)
- [ ] Remaining integer ISA: `mulld/mulli/divw/divd`, shifts (`slw/srw/sraw/srawi/sld/srd/rldic*`), `cntlzw`, `extsb/extsh/extsw`, `lbzx/lhzx/ldx/stbx/sthx/stdx`, update forms, `lmw/stmw`, `addic/subfic/adde/addze`, `nand/nor/andc/orc/eqv`, `crxor/cror/crand`, `mfcr/mtcrf`, `isync/sync/eieio`
- [ ] Atomics: `lwarx/stwcx./ldarx/stdcx.`
- [ ] FPU (`lfs/lfd/stfs/stfd/fadd/fmul/fmadd/fcmpu/frsp/fctiwz/...`) and FPSCR
- [ ] VMX/AltiVec (128-bit vector regs) -> SSE/AVX intrinsics
- [ ] Function discovery (use symbol info, `.opd`, call graph) so that code is emitted per function, not per chunk
- [ ] Parse `.sceStub` / PRX import tables, NID resolution; export tables
- [ ] Detect and extract embedded **SPU ELF** images (for Phase 4)
- Acceptance: `lift_report.txt` shows 0 unimplemented opcodes for a real homebrew sample, and it runs correctly.

## Phase 3 - PPU code -> normal x86-64 CPU (4 cores / 4 threads)
### 3a Correctness
- [x] Baseline: big-endian memory helpers, CR compare, `bc` semantics, LR/CTR, syscalls
- [ ] Full CR (SO bit), XER (CA/OV/SO), FPSCR modeling
- [ ] Differential testing against RPCS3's interpreter on tiny test ELFs
### 3b OS / library layer (HLE)
- [~] Syscalls: exit, tty_write
- [ ] Memory: `sys_memory_allocate/free`, `sys_mmapper_*`
- [ ] Threads/sync: `sys_ppu_thread_create/join/exit`, mutex, cond, semaphore, event queue/flag, `sys_timer_*`
- [ ] Filesystem: `cellFs*` mapped to the folder holding `EBOOT.BIN` (`PS3_GAME/USRDIR`, `/dev_hdd0` mapped to a local folder)
- [ ] Common PRX modules: `cellSysutil`, `cellPad`, `cellAudio`, `cellGcmSys`, `cellSpurs`, `cellSaveData`, `libc` shim
### 3c Performance and threading
- [ ] One host thread per PPU thread; scheduler capped by the **threads setting** (default 4 = quad-core, 4 threads)
- [ ] Function-level code generation, direct calls instead of switch dispatch, register caching, `-O2/-O3`
- [ ] Optional PGO/LTO in the build driver
- Acceptance: a multithreaded PPU test uses all configured threads and matches interpreter output.

## Phase 4 - SPU changes
- [ ] SPU ELF/`.sputext` extraction and lifting (128-bit registers, 256 KB local store, SPU ISA incl. shuffle/select)
- [ ] MFC DMA, mailboxes, signal notification, atomic (`getllar/putllc`) emulation
- [ ] `sys_spu_*` thread groups, SPURS/task libraries (HLE or lift the real SPURS kernel)
- [ ] Map SPU threads to host threads without starving PPU threads on a 4-thread CPU
- Acceptance: a homebrew SPU sample (e.g. vector add) produces correct output.

## Phase 5 - RSX -> DirectX 10, DirectX 11, Vulkan
- [x] Backend selector API (`ps3rt_set_graphics_backend`) and GUI setting
- [ ] GCM command buffer / FIFO parser, `cellGcmSys` HLE, display/flip queue
- [ ] NV47 state tracking (blend, depth, stencil, viewport, textures, surfaces)
- [ ] Shader translation: RSX vertex/fragment microcode -> HLSL (D3D10/D3D11) and SPIR-V (Vulkan)
- [ ] Texture formats/swizzle/Z-buffer formats, render targets
- [ ] Backend implementations in order: D3D11 -> Vulkan -> D3D10 (feature-level 10.0 limitations)
- Acceptance: homebrew triangle/texture samples render identically on all three backends.

## Phase 6 - Auto recompile into one exe + dll
- [x] Build driver: copies sources, writes `build.ninja`, runs ninja or sequential g++ (`project.cpp`)
- [x] Outputs `game.exe` + `ps3rt.dll` + `guest_image.bin`; GUI "Copy to EBOOT folder"
- [ ] MSVC backend using `Compilers-files/cl.exe` (+ `link.exe`, env setup)
- [ ] Optional C#-generated front-end code (currently only C/C++ is generated)
- [ ] Merge into a single self-contained exe option (static runtime, embedded `guest_image.bin` for small games)
- [ ] Incremental builds (only recompile changed chunks), parallel compile display in GUI
- [ ] Bundle verification: check `Compilers-files` contents on startup and report missing tools
- Acceptance: pasting the output next to `EBOOT.BIN` and starting `game.exe` runs without extra installs.
