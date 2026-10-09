# PPSX33 (PlayStation 3 Recompiler)

Static recompiler that turns a **decrypted PlayStation 3 `EBOOT.ELF`** (PowerPC 64 / Cell / RSX) into a native
**x86-64 Windows `game.exe` + `ps3rt.dll`** that you drop next to the original `EBOOT.BIN` and run.
Languages: **C++** (core, lifter, runtime), **C** (low-level helpers), **C#** (WinForms GUI).

> **Read this file first if you are an AI/agent continuing the project.** Section 9 is a hand-off checklist.
> Only use this with games you own and dumped yourself.

---

## 1. Honest status (what really works today)

| Phase | Goal | Status |
|------:|------|--------|
| 1 | UI + project folders | **Done (untested build)**: WinForms GUI written but *not compiled yet* (no .NET SDK was available when written). Native side is tested. |
| 2 | ELF decompile | **Partial**: ELF64-BE loader + PPU lifter for ~40 common instructions (see `docs/PPU_COVERAGE.md`). Current God of War III analysis: 1,285,560 instruction instances across 157 chunks, with 1,103,166 translated and 182,394 unimplemented. See [game coverage records](docs/games/README.md) and [GOW3 details](docs/games/GOW3.md). |
| 3 | PPU code -> x86-64 on a normal 4c/4t CPU | **Baseline works**: single-threaded generated C++, correct control flow, big-endian guest memory. Multithreading, fast paths, function splitting: TODO. |
| 4 | SPU | **TODO** (`runtime/spu_stub.cpp` placeholder). |
| 5 | RSX -> DirectX 10 / 11 / Vulkan | **TODO** (`runtime/rsx_stub.cpp` placeholder; backend selector exists). |
| 6 | Auto-compile to one exe + dll | **MSVC build succeeds** and produces `game.exe` + `ps3rt.dll`; runtime execution is still incomplete. |

Verified: `scripts/run_smoke_test.sh` builds a tiny hand-made PPC64 ELF (loop, `bl`/`blr`, `mtctr`/`bdnz`, compare,
`sc` syscall) -> lifts -> compiles `game` + `ps3rt` -> runs and prints `OK`. A separate MSVC build now succeeds and emits `game.exe` + `ps3rt.dll`; launching that output starts execution, then leaves the recompiled guest-code range. This is a runtime/control-flow failure, not a successful game run. No commercial game is confirmed playable.
Expect `ps3rt: unimplemented PPU instruction ...` first, then missing PRX/syscall HLE,
then SPU and RSX. Those are multi-year-scale problems (see RPCS3 for scale); the roadmap breaks them down.

## 2. User workflow (target)

1. Get a **decrypted ELF** from your own game (RPCS3 can decrypt `EBOOT.BIN`; the GUI has a helper button that opens RPCS3, nothing is played).
2. GUI -> **Load ELF** -> enter the **game name**.
3. App creates `<folder of PS3Recomp.exe>/<GameName>/` containing:
   - `input/`    the ELF (`EBOOT.elf`)
   - `codebase/` generated sources: `ppu_chunk_NNN.cpp`, `ppu_chunks.cpp`, `game_main.cpp`, `image_info.h`, `guest_image.bin`, `lift_report.txt`
   - `output/`   build tree + results: `src/`, `obj/`, `build.ninja`, **`game.exe`, `ps3rt.dll`, `guest_image.bin`**
4. **Decompile** -> **Build** -> **Copy to EBOOT folder** (copies `game.exe`, `ps3rt.dll`, `guest_image.bin`) -> run `game.exe`.

The optional MSVC tool subset is under `Compilers-files/MSCV/` (`cl.exe`, `link.exe`, and supporting files); see its README and `THIRD_PARTY.md` for setup and licensing notes. CMake, the Windows SDK, .NET 8, and fallback compiler tools are separate prerequisites.

## 3. Repository layout

```
PPSX33/
  README.md  ROADMAP.md  CMakeLists.txt  .gitignore
  Compilers-files/        optional MSVC compiler/linker support files (see README and THIRD_PARTY.md)
  docs/PPU_COVERAGE.md    instruction implementation notes and known gaps
  docs/games/README.md    index of games used for recompiler coverage work
  docs/games/GOW3.md      God of War III lift statistics and current coverage
  runtime/                SOURCE shipped next to the app; compiled per game into ps3rt.dll / game.exe
    ppu_runtime.h         PPUContext, big-endian memory helpers, branch helpers, ppu_run()  (header used by generated code)
    ps3rt.cpp             guest memory (4 GB virtual), guest_image.bin loader, HLE syscalls
    spu_stub.cpp          Phase 4 placeholder
    rsx_stub.cpp          Phase 5 placeholder + graphics backend selector
  src/core/               ps3core.dll (C++ and C)
    ps3_util.h/.c         C: byte swapping, FNV hash
    elf_loader.h/.cpp     ELF64 BE parser -> ElfImage (PT_LOAD segments)
    ppu_lifter.h/.cpp     PPU -> C++ generator, writes codebase/
    project.h/.cpp        project folders, lift step, build driver (build.ninja + g++/ninja)
    ps3core.h             C API exported to the GUI
    ps3core_api.cpp       C API implementation
  src/cli/ps3_cli.cpp     command-line front end (same API as GUI)
  src/gui/                C# WinForms GUI (.NET 8): MainForm, Native (P/Invoke), Settings, Rpcs3Launcher, PromptDialog
  tests/make_test_elf.py  builds the smoke-test ELF
  scripts/                build_all.bat, run_smoke_test.sh
```

## 4. Build the tool

Windows (needs CMake, Ninja, a C/C++ compiler, .NET 8 SDK):
```
scripts\build_all.bat        # -> build\dist\{PS3Recomp.exe, ps3core.dll, ps3_cli.exe, runtime\, Compilers-files\}
```
Linux/macOS quick test of the native pipeline: `scripts/run_smoke_test.sh` (needs g++ and python3).
The GUI finds `ps3core.dll`, `runtime/` and `Compilers-files/` next to its exe (`AppContext.BaseDirectory`).

## 5. How the recompiler works (design)

* **Guest memory**: one flat 4 GB virtual block; guest address `A` = `mem + A`. Data is big-endian in memory; every
  load/store goes through `rd8/16/32/64`, `wr*` (byte-swap). Registers are held natively (little-endian values).
* **Generated code**: each executable `PT_LOAD` segment is split into chunks of 8192 instructions. One C++ function per chunk:
  `bool ppu_chunk_N(PPUContext&)` containing `switch(pc){ case ADDR: ... }` with one case per instruction, falling through
  to the next. Direct branches inside a chunk are `pc = T; goto dispatch;`, outside the chunk `c.pc = T; return true;`.
  Indirect branches (`blr`, `bctr`) do the same with a runtime range check. `ppu_run()` picks the chunk covering `c.pc`.
  Unsupported opcode -> emits `ps3rt_unimplemented(...)` and halts, and it is counted in `codebase/lift_report.txt`
  (**that report is the TODO list for Phase 2**).
* **Entry point**: PS3 `e_entry` points to an OPD (function descriptor `{code address, TOC}`); the generator reads both
  (`image_info.h`: `kEntryPc`, `kEntryToc`; `r2` = TOC; `r1` = stack top at `0xD0000000+1MB`).
* **`guest_image.bin`** (little-endian): `"PS3IMG1\0"`, `u32 nseg`, `nseg * {u64 vaddr, u64 filesz, u64 memsz, u64 file_offset}`, then data.
  It is loaded by `ps3rt_init()` at startup and must sit next to `game.exe`.
* **Syscalls** (`sc`): `r11` = number, args `r3..r10`, result `r3`. Implemented: `22/41` exit, `403` tty_write. Others log and return 0.
* **Build driver** (`project.cpp: build_project`): copies `codebase` + `runtime` into `output/src` and emits the per-game build files. The MSVC path uses `cl.exe` and `link.exe` with the Visual Studio/Windows SDK environment when available; the fallback path can use a GCC-compatible compiler. Outputs `ps3rt.dll` (runtime) and `game.exe` (generated code linked against the DLL). A successful build does not guarantee correct guest execution.
* **C API** (`ps3core.h`): `ps3_create_project`, `ps3_lift_project`, `ps3_build_project`, `ps3_core_version`.

## 6. Known assumptions / limitations (verify!)

* Input must be an already **decrypted** ELF64 big-endian PPC64 (`e_machine == 21`). SELF/EBOOT.BIN decryption is delegated to RPCS3. How to automatically locate RPCS3's decrypted output is **not implemented** (user picks the ELF).
* PRX imports/exports (`sys_*`, `cell*` libraries via the PS3 import stubs/NIDs) are **not parsed yet**; games call these constantly (Phase 3b).
* Condition register: only LT/GT/EQ are modeled (SO ignored). XER carry/overflow not modeled. `Rc=1` supported on add/subf/or/and/xor/rlwinm.
* Windows commit of guest memory is per region on load; Linux relies on lazy `mmap`.
* `rlwinm` mask logic matches the 64-bit PowerPC definition but was only lightly tested.
* The C# GUI has **never been compiled**; expect small compile fixes on first build.
* `output/` dll name is `ps3rt.dll` on Windows and `ps3rt.so` on Linux (Linux only for dev/testing).

## 7. Conventions for contributors / AIs

* Keep generated code deterministic and readable; never hand-edit files under a game's `codebase/`.
* New instruction => edit `lift_one()` in `src/core/ppu_lifter.cpp`, add it to `docs/PPU_COVERAGE.md`, and extend `tests/make_test_elf.py` (or add a new test ELF) so `scripts/run_smoke_test.sh` still prints `PASSED`.
* Keep `ps3core.h` and `src/gui/Native.cs` in sync (signatures, buffer sizes).
* Do not commit game files, ELFs, keys or toolchain binaries.
* Update the status table in section 1 and `ROADMAP.md` checkboxes in the same commit as the work.

## 8. Roadmap summary (details + acceptance criteria in `ROADMAP.md`)

1. **Phase 1 UI** -> 2. **Phase 2 ELF decompile** -> 3. **Phase 3 PPU to x86-64 (4 cores / 4 threads)** -> 4. **Phase 4 SPU** -> 5. **Phase 5 RSX -> D3D10/D3D11/Vulkan** -> 6. **Phase 6 auto-recompile into one exe + dlls, paste next to EBOOT.BIN**.

## 9. Hand-off checklist (next steps, highest value first)

1. Compile the GUI on a machine with .NET 8; fix compile errors; test the full flow with `tests/make_test_elf.py` output.
2. Run the lifter on a real **homebrew** ELF (not a commercial game) and burn down `lift_report.txt` (ROADMAP Phase 2 / 3a).
3. Parse PS3 import tables (NID resolution) and add HLE module stubs (ROADMAP Phase 3b).
4. Add multithreading: PPU thread creation syscalls -> host threads, limited to the configured thread count.
5. Only then SPU (Phase 4), then RSX (Phase 5).

## 10. Project policies and contribution

- [Contributing](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md)
- [Support](SUPPORT.md)
- [Privacy](PRIVACY.md)
- [Governance](GOVERNANCE.md)
- [Third-Party Software and Assets](THIRD_PARTY.md)
- [License](LICENSE) (Apache License 2.0)
- [Citation metadata](CITATION.cff)

Bug reports and feature suggestions can be submitted using the repository's issue templates.
