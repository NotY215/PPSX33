# PPSX33 - PlayStation 3 Recompiler

PPSX33 is an experimental static recompiler for decrypted 64-bit big-endian PowerPC ELF input from PlayStation 3 software. Its Windows output includes a native `game.exe` and `ps3rt.dll`. The project is written in C++, C, and C#.

Use only game data and software that you own or are legally authorized to process.

## Project status

| Area | Status |
| --- | --- |
| WinForms application and project folders | Implemented; GUI build and full workflow validation remain in progress |
| ELF64 big-endian loading | Implemented |
| PPU instruction translation | Partial; see [PPU coverage](docs/PPU_COVERAGE.md) |
| Native MSVC build | Produces `game.exe` and `ps3rt.dll` |
| Runtime execution | Incomplete; current test starts and then leaves the recompiled guest-code range |
| SPU execution | Not implemented |
| RSX graphics rendering | Not implemented; backend selection exists |
| Full commercial-game compatibility | Not established |

A smoke test is available at `scripts/run_smoke_test.sh`. It exercises a small synthetic PPC64 ELF and is not a commercial-game compatibility test. The GoW3 lift record reports 1,285,560 instruction instances, of which 1,103,166 were translated and 182,394 remain unimplemented across 157 chunks. Static translation coverage is approximately 85.81%.

## Requirements

For the recommended Windows build environment, install the latest stable **Visual Studio 2026 (version 18)** from https://visualstudio.microsoft.com/downloads/ and select:

- **Desktop development with C++**, including the latest MSVC x64 tools and Windows SDK
- **.NET desktop development** and the targeting pack required by the GUI project, which currently targets `net8.0-windows`
- CMake and Ninja build tools

The current .NET/Visual Studio compatibility matrix is available at https://learn.microsoft.com/en-us/dotnet/core/install/windows.

See [Compiler Setup](Compilers-files/README.md) for the exact local tool staging layout and how to locate the installed MSVC files.

## Build on Windows

From a Visual Studio 2026 developer command prompt, at the repository root run:

```bat
scripts\build_all.bat
```

The build output is written to `build\dist\`. Launch the GUI with:

```bat
build\dist\PS3Recomp.exe
```

The native smoke test can be run on supported Linux/macOS development environments with `scripts/run_smoke_test.sh`, `g++`, and Python 3 installed.

## Recompile workflow

1. Prepare a decrypted ELF from software you are authorized to use.
2. Launch `PS3Recomp.exe` and select the ELF using **Load ELF**.
3. Enter a project name and create the project.
4. Run **Decompile**, then **Build**.
5. Inspect the generated files in the game's `output/` directory. The current Windows build path can produce `game.exe`, `ps3rt.dll`, and `guest_image.bin`.
6. Test the output and retain the runtime log when execution fails.

The generated executable currently does not establish game compatibility: the reported runtime test starts execution and then leaves the recompiled guest-code range. Unsupported instructions, system calls, PRX imports, SPU workloads, RSX graphics, and control-flow correctness remain areas of work.

## Repository layout

```text
PPSX33/
├── Compilers-files/        Local MSVC tool staging instructions
├── docs/
│   ├── PPU_COVERAGE.md     Implemented instruction families and gaps
│   └── games/              Recorded game lift measurements
├── runtime/                Runtime sources compiled for each output
├── src/
│   ├── core/               ELF loader, PPU lifter, project/build API
│   ├── cli/                Command-line front end
│   └── gui/                C# WinForms application
├── scripts/                Build and smoke-test scripts
└── tests/                  Synthetic ELF test generator
```

## Project information

- [Roadmap](ROADMAP.md)
- [Contributing](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md)
- [Support](SUPPORT.md)
- [Privacy](PRIVACY.md)
- [Governance](GOVERNANCE.md)
- [Third-Party Software and Assets](THIRD_PARTY.md)
- [Citation metadata](CITATION.cff)
