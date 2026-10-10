# PPSX33 Developer Guide

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="120" /></p>


This guide describes the repository layout, development workflow, and implementation responsibilities for contributors.

## Architecture overview

| Component | Location | Responsibility |
| --- | --- | --- |
| ELF loader | `src/core/elf_loader.cpp`, `src/core/elf_loader.h` | Parse ELF64 big-endian PowerPC64 inputs, loadable segments, entry points, symbols, and function descriptors |
| PPU lifter | `src/core/ppu_lifter.cpp`, `src/core/ppu_lifter.h` | Decode guest PPU instructions and emit translated C/C++ code |
| Project/build driver | `src/core/project.cpp`, `src/core/project.h` | Manage project files, invoke lifting, and coordinate host builds |
| Core API | `src/core/ps3core.h`, `src/core/ps3core_api.cpp` | Public native interface consumed by frontends |
| Runtime | `runtime/ppu_runtime.h`, `runtime/ps3rt.cpp` | Guest memory, CPU state helpers, and runtime support |
| SPU runtime | `runtime/ps3rt.cpp`, `runtime/spu_stub.cpp` | SPU context, loading/execution helpers, mailboxes, and MFC paths are in the runtime; ISA/SPURS coverage remains partial |
| RSX runtime | `runtime/rsx_stub.cpp` | Partial FIFO/method processing and D3D10/D3D11 integration; not a complete RSX implementation |
| CLI | `src/cli/ps3_cli.cpp` | Command-line entry point for project processing |
| GUI | `src/gui/` | C# WinForms interface and native interop |
| Synthetic tests | `tests/make_test_elf.py` | Generate a small PPC64 ELF for pipeline regression tests |
| PS3 sample | `tests/games/pong/pong.c` | PSL1GHT-based PS3 sample for the optional target toolchain pipeline |

## Build environment

Windows development requires Visual Studio 2022 or a compatible newer version with the C++ workload and Windows SDK, CMake, Ninja, and the .NET 8 SDK. See [Compiler Setup](../Compilers-files/README.md).

Build from the repository root in a Visual Studio developer command prompt:

```bat
scripts\build_all.bat
```

The output is placed in `build\dist\`. The GUI is published when the .NET SDK is available.

## Alpha packaging and release validation

The Alpha 01 package contains the WinForms application, native core library/import artifacts, CLI, .NET runtime metadata, logos, staged build tools, and selected runtime sources. The intended directory layout is documented in the root [README](../README.md), and the public-facing notes are in [PPSX33-Alpha-01 release notes](../releases/PPSX33-Alpha-01.md).

Before publishing a binary archive, validate the exact archive on a clean Windows machine. Check that the GUI starts, `ps3_cli.exe --help` returns its usage text, and the synthetic smoke test passes. If a check has not been run, identify it as unverified. Confirm all bundled third-party binaries can legally be redistributed.

## Test workflow

Run the synthetic smoke test after changes to the loader, lifter, runtime, project driver, or build pipeline.

Windows:

```bat
scripts\run_smoke_test.bat
```

Linux, macOS, or a compatible MSYS environment:

```sh
bash scripts/run_smoke_test.sh
```

The test creates a synthetic PPC64 ELF and expects the generated program to print `OK`. Record commands and results accurately. This test does not establish compatibility with commercial games.

The optional Pong workflow uses `scripts/build_pong_recompile.bat` or `scripts/build_pong_recompile.sh` and requires a compatible PSL1GHT toolchain plus a built PPSX33 CLI.

## Implementing a PPU opcode

1. Locate the relevant decode and emission path in `src/core/ppu_lifter.cpp`.
2. Check instruction fields and architectural effects against the PowerPC specification or a trusted implementation.
3. Review relevant helpers in `runtime/ppu_runtime.h`.
4. Implement the operation with explicit handling of register and condition state, record bits, carry/overflow, memory access, and endianness where applicable.
5. Add a focused synthetic test or regression case. Avoid validating an opcode only by counting it as translated.
6. Run the smoke test and all relevant available tests.
7. Re-run affected game lifts when authorized input is available, then update report-backed metrics in `docs/games/` and [PPU coverage](PPU_COVERAGE.md).
8. Document any behavior that remains approximate or unsupported.

Do not silently replace an unimplemented operation with a no-op and describe it as fully implemented.

## Changing the core API

When changing `src/core/ps3core.h`, update the implementation, CLI callers, and C# P/Invoke declarations in `src/gui/Native.cs`. Keep argument types, ownership, return values, and error handling consistent across the native and managed interfaces.

## Working with game coverage records

Game records must be based on real tool output. Store public-safe text reports only. Include the input build identifier when known, PPSX33 revision, workflow, and measured counts. Do not commit game executables, decrypted ELFs, EBOOT files, keys, or copyrighted assets.

Keep these metrics separate:

- Decode coverage
- Static translation coverage
- Semantic correctness
- Runtime correctness
- Full-game compatibility

See [game coverage records](games/README.md), [GOW3 record](games/GOW3/GOW3.md), and [Uncharted 2 analysis](games/Uncharted2/UNCHARTED2.md).

## Pull request checklist

- [ ] The change has a clear, focused scope.
- [ ] Related callers and interfaces have been updated.
- [ ] Relevant build and test commands have been run, or unrun checks are explicitly identified.
- [ ] New behavior has regression coverage where practical.
- [ ] Documentation and coverage records match verified behavior.
- [ ] No copyrighted game files, secrets, proprietary compiler binaries, or unrelated generated artifacts are included.
