# PPSX33: PlayStation 3 Static Recompiler

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-banner.png" alt="PPSX33 neon-blue banner" width="100%" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="220" />
</p>

<p align="center"><strong>PlayStation 3 Static Recompiler</strong><br />Decode · Translate · Play</p>

PPSX33 is an experimental static recompiler for decrypted ELF64 big-endian PowerPC64 input targeting the PlayStation 3. The repository contains a C/C++ core, command-line frontend, C# WinForms GUI, and guest runtime support. Visit the [PPSX33 project website](https://notyvos.gt.tc/PPSX3/) for the project page.

PPSX33 is not a complete PS3 emulator. PPU translation, correct instruction semantics, runtime behavior, system-library compatibility, SPU execution, and RSX rendering are separate engineering problems.

## Current implementation status

| Area | Status |
| --- | --- |
| ELF64 big-endian loading | Implemented for loadable segments and entry/function-descriptor handling |
| ELF analysis | Symbol and OPD discovery, heuristic PRX/NID detection, embedded SPU image detection |
| PPU static lifting | GOW3 / Minecraft / Uncharted 2 / Demon’s Souls / GTA SA / GTA V — see table below (static counts only) |
| PPU semantics | **PARTIAL (B)** — mul/div/rotate/carry helpers + LR-first recovery; lifted code still approximate in places |
| Project workflow | GUI and CLI support project creation, lifting, and host build invocation |
| Windows output | Build path can emit `game.exe`, `ps3rt.dll`, and `guest_image.bin` |
| Runtime correctness | Experimental; commercial-game execution is not established |
| Package (XX-A…G) | Game-named EXE, copy into `PS3_GAME/USRDIR`, legal/TERMS (system browser), focused pad logs — see [ROADMAP](ROADMAP.md) |
| GCM / cellFs (XX-H…J) | GCM HLE (`gcm_hle`), null-loop recovery, PSARC TOC + zlib block streaming (`assets.cpp`) |
| SPU / MFC / SPURS (XX-K…M) | SPU ISA interpreter, MFC put/get/list/tags, SPURS-like job queue + `cellSpurs` HLE (8 SPU slots) — still a subset |
| Audio / UI (XX-N) | `cellAudio` → host waveOut; GDI status overlay; window title uses game name |
| ICON0 (XX-O) | `PS3_GAME/ICON0.PNG` applied as window/taskbar icon when run from USRDIR |
| RSX | **PARTIAL (A)** — surface descriptors, texture unit state, guest color-buffer blit to D3D11, expanded methods; full shaders/raster still missing |

### Post–XX-O lettered implementations

| ID | Focus | Status |
|----|--------|--------|
| **A** | RSX surfaces + color blit + method expansion | DONE in tree (`runtime/rsx_stub.cpp`) |
| **B** | PPU accuracy helpers + recovery preference | DONE in tree (`ppu_runtime_core_*.h`, `ppu_runtime_run.h`) |

See [ROADMAP.md](ROADMAP.md) § *Future work (post XX-O)* for remaining gaps.

### Game analysis snapshots

| Game | Entry OPD | OPDs detected | PRX hits | Embedded SPU images | Chunks |
| --- | --- | ---: | ---: | ---: | ---: |
| God of War III | `0x50ddc0` | 3 | 47 | 8 | 157 |
| Minecraft (PS3) | `0xbbe520` | 4,097 | 45 | 11 | 371 |
| Uncharted 2: Among Thieves | `0xdd8618` | 116 | 391 | 7 | 426 |
| Demon’s Souls | `0x1916138` | 4,097 | 134 | 12 | 775 |
| Grand Theft Auto: San Andreas | `0x77b718` | 4,097 | 23 | 0 | 232 |
| Grand Theft Auto V | `0x1a90b80` | 4,097 | 56 | 8 | 847 |

All counts are static-lift snapshots, not playability proof. See [PPU coverage](docs/PPU_COVERAGE.md), [game records](docs/games/README.md), and [roadmap](ROADMAP.md).

## Current runtime development status

[ROADMAP.md](ROADMAP.md) tracks tooling (I–XIX), packaging (XX-A…G), runtime playability (XX-H…O), and post-O lettered work (**A** RSX, **B** PPU). Milestone labels describe engineering work, not verified commercial-title compatibility.

**A (RSX):** color/depth surface state, texture unit 0, COLOR_OFFSET/PITCH/FORMAT methods, guest surface blit on flip.  
**B (PPU):** mulhd/mulhdu, div edge cases, rlwinm/rld*, addc/adde/subfc, FPSCR sticky, CR helpers; recovery prefers LR/stack over region-skip.

Keep all split PPU headers together when packaging the runtime. Additional code lives in `runtime/ps3rt.cpp`, `ps3rt_b.inl`, `spu_stub.cpp`, `assets.cpp`, `audio_save.cpp`, and `rsx_stub.cpp`.

## Documentation

- [USER_MANUAL.md](USER_MANUAL.md) — end-user workflow, legal notice, troubleshooting  
- [ROADMAP.md](ROADMAP.md) — full phase and gap list  
- [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) — architecture for contributors  

## Project website and releases

- **Project website:** [notyvos.gt.tc/PPSX3](https://notyvos.gt.tc/PPSX3/)
- **Release documentation:** [releases/README.md](releases/README.md)
- **Alpha 01 notes:** [releases/PPSX33-Alpha-01.md](releases/PPSX33-Alpha-01.md)
- **Release prerequisites:** [Redist.md](Redist.md)

## Requirements

Windows development uses CMake, Ninja, a C++17-capable compiler, and the .NET 8 SDK for the GUI. Visual Studio with **Desktop development with C++**, a Windows SDK, and .NET 8 is the documented setup.

## Build on Windows

```bat
scripts\\build_all.bat
```

Output under `build\\dist\\`. Launch GUI with `build\\dist\\PS3Recomp.exe`.

## Recompile workflow

1. Use an ELF you are authorized to analyze.  
2. Load ELF or Decrypt EBOOT → Decompile → (controller map) → Build → Copy to game folder → Run from USRDIR.  
3. Successful compilation is not proof of runtime correctness.

## Repository map

```text
Compilers-files/  Compiler discovery and local tool staging guidance
docs/             Developer guide, key map, PPU coverage, game lift records
runtime/          Guest memory, PPU helpers, logging, audio/save, SPU/RSX support
src/core/         ELF loader, PPU lifter, project/build driver
src/cli/          Command-line frontend
src/gui/          C# WinForms GUI
scripts/          Build, smoke-test, restore scripts
tests/            Synthetic ELF and Pong sample
releases/         Alpha package notes
```

## Developer documentation and policies

- [Contributor workflow](CONTRIBUTING.md)
- [Developer guide](docs/DEVELOPER_GUIDE.md)
- [Roadmap](ROADMAP.md)
- [User manual](USER_MANUAL.md)
- [Security policy](SECURITY.md)
- [Support](SUPPORT.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
