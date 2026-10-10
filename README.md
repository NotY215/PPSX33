# PPSX33: PlayStation 3 Static Recompiler

PPSX33 is an experimental static recompiler for decrypted ELF64 big-endian PowerPC64 input targeting the PlayStation 3. The repository contains a C/C++ core, command-line frontend, C# WinForms GUI, and guest runtime support.

PPSX33 is not a complete PS3 emulator. PPU translation, correct instruction semantics, runtime behavior, system-library compatibility, SPU execution, and RSX rendering are separate engineering problems.

## Current implementation status

| Area | Status |
| --- | --- |
| ELF64 big-endian loading | Implemented for loadable segments and entry/function-descriptor handling |
| ELF analysis | Symbol and OPD discovery, heuristic PRX/NID detection, embedded SPU image detection |
| PPU static lifting | GOW3: 1,285,560 translated instances, zero reported unimplemented instances, 157 chunks. Uncharted 2: 3,485,016 translated instances, zero reported unimplemented instances, 426 chunks |
| PPU semantics | Partial; some operations are approximate or use no-op fallbacks |
| Project workflow | GUI and CLI support project creation, lifting, and host build invocation |
| Windows output | Build path can emit `game.exe`, `ps3rt.dll`, and `guest_image.bin` |
| Runtime correctness | Experimental; commercial-game execution is not established |
| SPU | Context and API stubs exist; full instruction execution is not implemented |
| RSX | Backend selection exists; GCM/RSX rendering is not implemented |

Both game counts describe individual static-lift snapshots. They are not proof of correct instruction semantics, successful boot, rendering, or playability. See [PPU coverage](docs/PPU_COVERAGE.md), [game records](docs/games/README.md), and [roadmap](ROADMAP.md).

## Requirements

Windows development uses CMake, Ninja, a C++17-capable compiler, and the .NET 8 SDK for the GUI. Visual Studio 2022 with **Desktop development with C++**, a Windows SDK, and .NET 8 is a supported setup. Newer Visual Studio versions may work, but the build must be verified in that environment.

- [Visual Studio](https://visualstudio.microsoft.com/downloads/)
- [CMake](https://cmake.org/)
- [Ninja](https://github.com/ninja-build/ninja)
- [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0)

See [Compiler Setup](Compilers-files/README.md) for tool discovery and local staging rules.

## Build on Windows

Open a Visual Studio developer command prompt with MSVC, CMake, Ninja, and .NET 8 available on `PATH`. From the repository root:

```bat
scripts\build_all.bat
```

Output is placed in `build\dist\`. When the GUI target is available, launch it with:

```bat
build\dist\PS3Recomp.exe
```

If the .NET SDK is unavailable, CMake may skip the GUI target.

## Run the synthetic smoke test

On Linux, macOS, or a compatible MSYS environment with Python 3 and a C++17 compiler:

```sh
bash scripts/run_smoke_test.sh
```

On Windows, build first, then run:

```bat
scripts\run_smoke_test.bat
```

The smoke test creates a synthetic PPC64 ELF, runs the lift/build pipeline, and expects the generated program to print `OK`. It checks the pipeline, not commercial-game compatibility.

## Recompile workflow

1. Use an ELF file that is legally authorized for analysis.
2. Launch `build\dist\PS3Recomp.exe` and select **Load ELF**, or use **Find decrypted ELF** with a configured RPCS3 installation.
3. Create a project. The expected layout is `<root>/<game>/{input,codebase,output}`.
4. Run **Decompile / Recompile to C++**.
5. Inspect `codebase\lift_report.txt` and `codebase\analysis_report.txt`.
6. Run **Build exe + dll** and inspect artifacts in `output\`.
7. Validate runtime behavior separately from successful compilation. Preserve logs and exact reproduction steps for failures.

CLI syntax:

```text
ps3_cli <root_dir> <game_name> <elf_path> <runtime_dir> <compilers_dir>
```

The final arguments identify runtime sources and compiler tools. The CLI performs project creation, lifting, and build steps.

## Sample source

`tests/games/pong/pong.c` is a PS3-targeted PPU/VMX sample using PSL1GHT RSX/GCM APIs. It is not a desktop Pong application. The helper scripts attempt to compile it to a PS3 ELF and run the PPSX33 pipeline. This requires a compatible PSL1GHT toolchain and is experimental unless a successful test result is recorded.

## Repository map

```text
Compilers-files/  Compiler setup and local tool staging
docs/             PPU coverage and game lift records
runtime/          Guest memory, PPU helpers, SPU stubs, runtime support
src/core/         ELF loader, PPU lifter, project/build driver, C API
src/cli/          Command-line frontend
src/gui/          C# WinForms frontend
scripts/          Build, smoke-test, and Pong pipeline scripts
tests/            Synthetic ELF generator and PS3 Pong sample
```

## Developer documentation and policies

- [Contributor workflow](CONTRIBUTING.md)
- [Developer guide](docs/DEVELOPER_GUIDE.md)
- [PPU coverage methodology](docs/PPU_COVERAGE.md)
- [Game coverage records](docs/games/README.md)
- [Roadmap](ROADMAP.md)
- [Third-party software](THIRD_PARTY.md)
- [Security policy](SECURITY.md)
- [Support](SUPPORT.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Governance](GOVERNANCE.md)
- [Privacy](PRIVACY.md)
- [Citation metadata](CITATION.cff)
