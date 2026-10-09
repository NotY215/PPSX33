# PPSX33: PlayStation 3 Static Recompiler

PPSX33 is an experimental static recompiler that accepts decrypted 64-bit big-endian PowerPC ELF input and generates C++ code and native host build artifacts. The project includes a C/C++ core, a command-line frontend, a C# WinForms GUI, and a portable runtime.

PPSX33 is not a complete PlayStation 3 emulator. PPU translation coverage, runtime semantics, PS3 system-library compatibility, SPU execution, and RSX rendering are separate work areas.

Use only software and game data you own or are legally authorized to process.

## Current status

| Area | Current state |
| --- | --- |
| ELF64 big-endian loading | Implemented, including loadable segments and entry/OPD handling |
| ELF analysis | Symbol and OPD discovery, PRX/NID string heuristics, and embedded SPU image detection |
| PPU static lifting | GOW3 snapshot reports 1,285,560 translated instruction instances and zero unimplemented instances across 157 chunks |
| Instruction semantics | Uneven; some less common operations are approximate or treated as no-ops. Static translation is not semantic validation |
| Project workflow | GUI and CLI can create project folders, lift code, and invoke a native build |
| Windows artifacts | Build path can emit `game.exe`, `ps3rt.dll`, and `guest_image.bin` |
| Runtime | Experimental; correct execution of a full commercial game is not established |
| SPU | Runtime context and API stubs exist; full SPU ISA execution is not implemented |
| RSX | Graphics backend selection exists, but GCM/RSX rendering is not implemented |
| Commercial-game compatibility | Not established |

See [PPU coverage](docs/PPU_COVERAGE.md), the [GOW3 lift record](docs/games/GOW3/GOW3.md), and the [roadmap](ROADMAP.md).

## Requirements

For Windows development, use CMake, Ninja, a C++17-capable compiler, and the .NET 8 SDK for the GUI. Visual Studio 2022 with **Desktop development with C++**, the Windows SDK, and .NET 8 is a supported setup. Newer Visual Studio installations may also work, but the project build scripts and tool discovery should be verified in that environment.

- [Visual Studio downloads](https://visualstudio.microsoft.com/downloads/)
- [CMake](https://cmake.org/)
- [Ninja](https://github.com/ninja-build/ninja)
- [.NET 8](https://dotnet.microsoft.com/download/dotnet/8.0)

See [Compiler Setup](Compilers-files/README.md) for local MSVC tool staging and licensing notes.

## Build on Windows

Open a Visual Studio developer command prompt with CMake, Ninja, MSVC, and .NET 8 available on `PATH`. From the repository root run:

```bat
scripts\build_all.bat
```

The build output is placed in `build\dist\`. Launch the GUI with:

```bat
build\dist\PS3Recomp.exe
```

The native core and CLI are CMake targets. If the .NET SDK is not found, CMake warns that the GUI target will be skipped.

## Run the synthetic smoke test

On Linux, macOS, or a compatible MSYS environment with Python 3 and a C++17 compiler:

```sh
bash scripts/run_smoke_test.sh
```

On Windows, build first and then run:

```bat
scripts\run_smoke_test.bat
```

The smoke test creates a small synthetic PPC64 ELF, runs the lift/build pipeline, and expects the generated program to print `OK`. It is a pipeline regression test, not a commercial-game test.

## Recompile workflow

1. Use a decrypted ELF that you are legally authorized to process.
2. Open `build\dist\PS3Recomp.exe` and select **Load ELF**, or use **Find decrypted ELF** after configuring and running your own RPCS3 installation.
3. Enter a project name. PPSX33 creates `input\`, `codebase\`, and `output\` folders.
4. Run **Decompile / Recompile to C++**.
5. Review `codebase\lift_report.txt` and `codebase\analysis_report.txt`.
6. Run **Build exe + dll** and inspect the generated artifacts in `output\`.
7. Treat successful compilation as an intermediate result. Test runtime behavior separately and retain logs for reproducible failures.

The CLI accepts:

```text
ps3_cli <root_dir> <game_name> <elf_path> <runtime_dir> <compilers_dir>
```

The final two arguments identify the runtime source directory and compiler-tool directory. The CLI runs project creation, lifting, and building in sequence.

## Sample source

`tests/games/pong/pong.c` is a PS3-targeted PPU/VMX sample that uses PSL1GHT RSX/GCM APIs for a framebuffer Pong demo. It is not a native desktop Pong program. The helper scripts in `scripts/` attempt to compile the sample to an ELF and run it through PPSX33. That path requires a compatible PSL1GHT toolchain and has not been represented as validated unless a test result says so.

## Repository layout

```text
PPSX33/
├── Compilers-files/        Local compiler-tool staging instructions
├── docs/                   Coverage and game lift records
├── runtime/                Guest memory, PPU helpers, SPU stubs, and runtime support
├── src/core/               ELF loader, PPU lifter, project/build driver, C API
├── src/cli/                Command-line frontend
├── src/gui/                C# WinForms frontend
├── scripts/                Build, smoke-test, and Pong pipeline scripts
└── tests/                  Synthetic ELF generator and PS3 Pong sample
```

## Project policies

- [Roadmap](ROADMAP.md)
- [Contributing](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security](SECURITY.md)
- [Support](SUPPORT.md)
- [Privacy](PRIVACY.md)
- [Governance](GOVERNANCE.md)
- [Third-party software](THIRD_PARTY.md)
- [Citation metadata](CITATION.cff)
