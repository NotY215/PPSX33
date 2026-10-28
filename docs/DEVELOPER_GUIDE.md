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

Windows development uses Visual Studio 2026 (version 18) with the Desktop development with C++ workload and a Windows SDK, plus CMake, Ninja, and the .NET 8 SDK. Other supported toolchain versions must be validated in CI or with the repository smoke test. See [Compiler Setup](../Compilers-files/README.md).

Build from the repository root in a Visual Studio developer command prompt:

```bat
scripts\build_all.bat
```

The output is placed in `build\dist\`. The GUI is published when the .NET SDK is available. The current WinForms frontend includes ELF loading, an optional launcher for the separately installed RPCS3 EBOOT `--decrypt` workflow, project decompilation, object-file-based build progress/ETA, run/stop, output copying, persisted graphics/thread/controller settings, and build-cancellation UI. This integration does not bundle RPCS3 or determine whether the user's chosen input and intended processing are legally permitted.

## Alpha packaging and release validation

The Alpha 01 package contains the WinForms application, native core library/import artifacts, CLI, .NET runtime metadata, logos, staged build tools, and selected runtime sources. The intended directory layout is documented in the root [README](../README.md), and the public-facing notes are in [PPSX33-Alpha-01 release notes](../releases/PPSX33-Alpha-01.md).

Before publishing a binary archive, validate the exact archive on a clean Windows machine. Audit each included image, icon, font, runtime library, and other third-party artifact for provenance, redistribution permission, and required notices; repository presence does not establish a license. Audit each included image, icon, font, runtime library, and other third-party artifact for provenance, redistribution permission, and required notices; do not assume repository presence establishes a license. Check that the GUI starts, `ps3_cli.exe --help` returns its usage text, and the synthetic smoke test passes. If a check has not been run, identify it as unverified. Confirm all bundled third-party binaries can legally be redistributed.

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

## Runtime header layout and maintenance

The PPU runtime is split into the following headers. All must remain available to the compiler and be copied into generated build output by the project build driver:

| File | Responsibility |
| --- | --- |
| `runtime/ppu_runtime.h` | Thin public include wrapper for the runtime core and execution loop |
| `runtime/ppu_runtime_core.h` | Shared PPU context and common runtime definitions |
| `runtime/ppu_runtime_core_a.h` | First group of runtime helper definitions |
| `runtime/ppu_runtime_core_b.h` | Additional memory, XER, condition-register, and FPSCR helpers |
| `runtime/ppu_runtime_run.h` | PPU chunk dispatch, external-call recovery, and null-loop escape logic |
| `runtime/ps3rt.cpp` | Guest memory setup, LV2/HLE support, SPU API paths, and GCM state handling |
| `runtime/rsx_stub.cpp` | Partial RSX command and rendering support |
| `runtime/spu_stub.cpp` | SPU-related fallback/stub integration |

Do not restore the older single-header runtime as a routine fix. If a header is moved, renamed, or added, update `src/core/project.cpp` so every required runtime file is copied into the generated project output, then run the smoke test and a clean build.

## Runtime milestone status

Use the current [roadmap](../ROADMAP.md) as the source of truth for runtime and playability milestones. Phases I–XIX are **testing**; packaging XX-A…G and runtime playability XX-H…O are **done in tree**. Milestone labels describe engineering work, not verified commercial-game compatibility.

The runtime contains experimental GCM/HLE, PPU recovery, SPU interpreter/MFC/SPURS, cellFs/PSARC, audio, and RSX present paths. These are incomplete compatibility layers. Recovery logic may redirect control flow, and a translated instruction count does not establish semantic correctness or playability. Record exact logs and tests when making claims about guest behavior.


## GUI workflow, status, and cancellation

The GUI creates projects beneath `build/dist/projects/<name>/` with `input/`, `codebase/`, and `output/` directories. **Load ELF** accepts a decrypted ELF/SELF; **Decrypt EBOOT** delegates decryption to the selected RPCS3 executable. Decompilation writes generated C++ plus `lift_report.txt` and `analysis_report.txt` into `codebase/`. The build driver stages runtime and game sources under `output/src/`, invokes the configured host toolchain, and writes output binaries and reports beneath `output/`.

Build progress in the GUI is inferred by comparing object-file timestamps while the build is running. It is an advisory estimate, not authoritative compiler progress. A successful operation should display an operation-specific success state; failed and stopped operations should remain distinct in the log/status.

The Stop/Cancel control has context-sensitive behavior: it stops the launched game while a game process is active, and during a build it asks for confirmation before calling the native build-cancel API. The GUI's cancelled-build path removes generated `input/`, `codebase/`, and `output/` folders and clears the active project, so the user must reload the ELF. Do not describe Decompile as cancellable: the current GUI calls the native lift API synchronously and has no matching cancellation API for it. The native build cancellation flag is cooperative and should be tested against actual compiler subprocess behavior before claiming immediate termination.

The settings model is in `src/gui/Settings.cs`; its file is `%APPDATA%/PPSX33/settings.json`. `src/gui/KeyMap.cs` defines defaults, `MainForm.Controller.cs` edits and writes the runtime pad map, and `MainForm.Decrypt.cs` integrates the external RPCS3 launcher. Keep GUI behavior descriptions synchronized with these files and `MainForm.cs`.

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

Game records must be based on real tool output. Store only public-safe, sanitized text reports. Include the input build identifier when known, PPSX33 revision, workflow, and measured counts. Before committing logs, remove local user paths, usernames, proprietary strings or data, secrets, and unnecessary input-derived details. Do not commit game executables, decrypted ELFs, EBOOT files, keys, extracted game assets, or third-party artwork without verified permission.

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

## Phase XX runtime modules (H–O)

When updating a checkout after Phase XX work, keep these files in sync as a set:

| Module | Path | Role |
|--------|------|------|
| PPU loop / null-loop | `runtime/ppu_runtime_run.h` | Soft-break / region skip (XX-H) |
| GCM + SPURS HLE | `runtime/ps3rt_b.inl`, `runtime/nid_table.h` | `gcm_hle`, `spurs_dispatch` (XX-I, XX-M) |
| Assets / PSARC | `runtime/assets.cpp` | cellFs + zlib block streaming (XX-J) |
| SPU interpreter | `runtime/spu_stub.cpp` | ISA step, MFC, SPURS schedule (XX-K–M) |
| Runtime shell | `runtime/ps3rt.cpp` | SPU pool (8), run/tick/MFC API |
| Context | `runtime/ppu_runtime_core_a.h` | `SPUContext` MFC/SPURS fields |
| LV2 extras | `runtime/lv2_syscalls.inl` | `sys_spu_thread_*` range |
| Audio / save | `runtime/audio_save.cpp` | cellAudio→waveOut (XX-N) |
| RSX host | `runtime/rsx_stub.cpp` | Present, GDI overlay (XX-N), ICON0 (XX-O) |

### Host dependencies (Windows)

- **MSVC** toolset as used by the GUI build path  
- **zlib1.dll** (or `zlib.dll`) next to the game EXE for compressed PSARC blocks  
- **waveOut** (winmm) for XX-N audio  
- **WIC** for ICON0 PNG decode  

### Log tags (quick reference)

| Tag | Phase |
|-----|-------|
| `[gcm] XX-I` | GCM HLE |
| `[psarc] XX-J` / `[fs] XX-J` | Archives and cellFs |
| `[spu] XX-K` / `XX-L` | Interpreter / MFC |
| `[spurs] XX-M` | Job schedule |
| `[audio] XX-N` | Audio init / AddData |
| `[rsx] XX-O` | Icon / window title |

See the root [ROADMAP.md](../ROADMAP.md) for goals, status, and the symptom guide.
