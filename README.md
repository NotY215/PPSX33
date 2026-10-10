# PPSX33: PlayStation 3 Static Recompiler

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-banner.png" alt="PPSX33 neon-blue banner" width="100%" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="220" />
</p>

<p align="center"><strong>PlayStation 3 Static Recompiler</strong><br />Decode · Translate · Play</p>

PPSX33 is an experimental static recompiler for decrypted ELF64 big-endian PowerPC64 input targeting the PlayStation 3. The repository contains a C/C++ core, command-line frontend, C# WinForms GUI, and guest runtime support.

PPSX33 is not a complete PS3 emulator. PPU translation, correct instruction semantics, runtime behavior, system-library compatibility, SPU execution, and RSX rendering are separate engineering problems.

## Current implementation status

| Area | Status |
| --- | --- |
| ELF64 big-endian loading | Implemented for loadable segments and entry/function-descriptor handling |
| ELF analysis | Symbol and OPD discovery, heuristic PRX/NID detection, embedded SPU image detection |
| PPU static lifting | GOW3: 1,285,560 translated, 0 reported unimplemented, 157 chunks. Minecraft: 3,034,530 translated, 0 reported unimplemented, 371 chunks. Uncharted 2: 3,485,016 translated, 0 reported unimplemented, 426 chunks. Demon's Souls: 6,343,506 translated, 0 reported unimplemented, 775 chunks. GTA: San Andreas: 1,893,428 translated, 0 reported unimplemented, 232 chunks. GTA V: 6,934,214 translated, 0 reported unimplemented, 847 chunks |
| PPU semantics | Partial; some operations are approximate or use no-op fallbacks |
| Project workflow | GUI and CLI support project creation, lifting, and host build invocation |
| Windows output | Build path can emit `game.exe`, `ps3rt.dll`, and `guest_image.bin` |
| Runtime correctness | Experimental; commercial-game execution is not established |
| SPU | Runtime interpreter and create/load/run/mailbox/MFC paths exist in `runtime/ps3rt.cpp`; full ISA coverage, vector/FP behavior, and SPURS remain incomplete |
| RSX | Partial FIFO/method decoding, surface and draw tracking, flip/control handling, and D3D10/D3D11 integration are present; complete RSX rendering compatibility is not established |

### Game analysis snapshots

| Game | Entry OPD | OPDs detected | PRX hits | Embedded SPU images | Chunks |
| --- | --- | ---: | ---: | ---: | ---: |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/God_of_War_Logo.png" width="86" alt="God of War logo" /> God of War III | `0x50ddc0` | 3 | 47 | 8 | 157 |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Minecraft_Logo.png" width="86" alt="Minecraft logo" /> Minecraft (PS3) | `0xbbe520` | 4,097 | 45 | 11 | 371 |
| <img src="https://www.pngkey.com/png/detail/983-9839044_uncharted-2-among-thieves.png" width="48" alt="Uncharted 2 logo artwork" /> Uncharted 2: Among Thieves | `0xdd8618` | 116 | 391 | 7 | 426 |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Demon%27s_Souls_logo_black.svg" width="86" alt="Demon's Souls logo" /> Demon's Souls | `0x1916138` | 4,097 | 134 | 12 | 775 |
| <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_San_Andreas_logo.svg" width="86" alt="Grand Theft Auto: San Andreas logo" /> Grand Theft Auto: San Andreas | `0x77b718` | 4,097 | 23 | 0 | 232 |
| <img src="https://thumb.wikimedia.org/wikipedia/it/thumb/7/76/Grand_Theft_Auto_V_logo.svg/330px-Grand_Theft_Auto_V_logo.svg.png?utm_source=it.wikipedia.org&utm_campaign=index&utm_content=thumbnail&_=20240305183247" width="86" alt="Grand Theft Auto V logo" /> Grand Theft Auto V | `0x1a90b80` | 4,097 | 56 | 8 | 847 |

All four game counts describe individual static-lift snapshots. They are not proof of correct instruction semantics, successful boot, rendering, or playability. Grand Theft Auto: San Andreas now has a recorded static-lift and ELF-analysis snapshot: 1,893,428 instruction instances translated, 0 reported unimplemented, 232 chunks, 5 PT_LOAD segments, entry OPD `0x77b718`, 4,097 OPD entries, 23 PRX/module string hits, and 0 embedded SPU images detected. GTA V now also has a recorded snapshot: 6,934,214 instruction instances translated, 0 reported unimplemented, 847 chunks, 5 PT_LOAD segments, entry OPD `0x1a90b80`, 4,097 OPD entries, 56 PRX/module string hits, and 8 embedded SPU images detected. These counts do not establish semantic correctness or playability. See [PPU coverage](docs/PPU_COVERAGE.md), [game records](docs/games/README.md), and [roadmap](ROADMAP.md).

## Alpha release: PPSX33-Alpha-01

PPSX33-Alpha-01 is an experimental Windows package for early evaluation of the GUI, CLI, static ELF analysis, PPU lifting, and project build workflow. It is not a production release and does not claim full PS3 game compatibility.

The release package is organized as follows:

```text
PPSX33-Alpha-01/
  ppsx33-logo.ico
  ppsx33-logo.png
  ps3core.dll
  ps3core.exp
  ps3core.lib
  PS3Recomp.deps.json
  PS3Recomp.dll
  PS3Recomp.exe
  PS3Recomp.pdb
  PS3Recomp.runtimeconfig.json
  ps3_cli.exe
  Compilers-files/
    cmake.exe
    ninja.exe
    README.md
    MSCV/
  runtime/
    boot_continue.patch
    nid_table.h
    ppu_runtime.h
    ps3rt.cpp
    rsx_stub.cpp
    spu_stub.cpp
```

The GUI targets .NET 8 for Windows and is framework-dependent, so install the .NET 8 Desktop Runtime if it is not already available. A complete package should be smoke-tested on a clean Windows environment before publishing. The presence of compiler files does not imply that all Microsoft build tools can legally be redistributed; include MSVC binaries only when their applicable license permits redistribution. Do not include game executables, decrypted ELFs, keys, or other copyrighted game data.

See the [Alpha 01 release notes](releases/PPSX33-Alpha-01.md) for scope, known limitations, and validation guidance.

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
