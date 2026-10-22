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
| PPU static lifting | High translate counts on samples; **XXV-E** treats unknown encodings as unsupported (not silent success) |
| PPU semantics | Partial; honesty path in XXV-C/E; full opcode audit continues |
| Project workflow | GUI and CLI: project create, lift, host build |
| Windows output | Project EXE, `ps3rt.dll`, `guest_image.bin` |
| Runtime correctness | Experimental; commercial playability not established |
| Package (XX–XXIV) | Packaging, GCM/cellFs, SPU/MFC subset, audio host path — see [ROADMAP](ROADMAP.md) |
| RSX (XXV-D/E) | D3D11 present; clear→draw→Present; FIFO `state_apply` / host draw; guest color/tex blit; solid-triangle fallback; vertex/depth/blend **notes** only — **not** full RSX shader ISA |
| Phase XXII–XXIV | Finished / in tree — see roadmap |
| Phase XXV | **Active** — [roadmap/XXV.md](roadmap/XXV.md) |

### Game analysis snapshots

Static-lift counts are historical snapshots only. They are **not** proof of correct instruction semantics, successful boot, rendering, or playability. See [PPU coverage](docs/PPU_COVERAGE.md), [game records](docs/games/README.md), and [roadmap](ROADMAP.md).

## Current runtime development status

The current [ROADMAP.md](ROADMAP.md) tracks tooling (**I–XIX**), packaging (**XX**), lettered **XXI**, pipeline **XXII**, execution reality **XXIII**, boot assist **XXIV**, and active **XXV**.

### Phase XXV (active)

| ID | Status | Focus |
|----|--------|--------|
| **A–B** | DONE (untested) | Audio host build fix; visible D3D11 present |
| **C / E** | DONE (untested) | Lifter honesty; present order; solid triangle |
| **D** | DONE (untested) | Full depth: FIFO decode, surfaces, host draw, GCM seed, vertex/depth notes |
| **F** | PLANNED | Strict guest faults |
| **G–J** | PLANNED | PRX/LV2, SPU/MFC, services, E2E proof |

**Suggested patch order:** XXV-E pack, then XXV-D full-depth-on-E pack. Detail: [roadmap/XXV.md](roadmap/XXV.md).

**XXV-D/E host features (not game-accurate RSX):**

- Present path fixed (clear, then draw, then Present)
- Guest color/texture upload + HLSL fullscreen quad when a surface is bound
- Solid-color triangle fallback so the window is never a dead clear
- FIFO method apply table; draw methods call host draw
- Optional GCM-seeded color buffer when guest PUT does not advance
- Lifter reports unsupported encodings instead of counting them implemented

Approach: capture failing PC/NID/method → define semantics → implement real behaviour → re-test.

The PPU runtime is split across `runtime/ppu_runtime.h`, `runtime/ppu_runtime_core.h`, `runtime/ppu_runtime_core_a.h`, `runtime/ppu_runtime_core_b.h`, and `runtime/ppu_runtime_run.h`. Keep all split headers together when building or packaging the runtime. Additional playability code lives in `runtime/ps3rt.cpp`, `runtime/ps3rt_b.inl`, `runtime/spu_stub.cpp`, `runtime/assets.cpp`, `runtime/audio_save.cpp`, and `runtime/rsx_stub.cpp`.

## Project website and releases

- **Project website:** [notyvos.gt.tc/PPSX3](https://notyvos.gt.tc/PPSX3/)
- **Release documentation:** [releases/README.md](releases/README.md)
- **Alpha 01 notes:** [releases/PPSX33-Alpha-01.md](releases/PPSX33-Alpha-01.md)
- **Release prerequisites:** [Redist.md](Redist.md)

## Alpha release: PPSX33-Alpha-01

PPSX33-Alpha-01 is an experimental Windows package for early evaluation of the GUI, CLI, static ELF analysis, PPU lifting, and project build workflow. It is not a production release and does not claim full PS3 game compatibility. See [Alpha 01 release notes](releases/PPSX33-Alpha-01.md) and [Redist.md](Redist.md).

## GUI status and settings

The GUI workflow is **Decrypt EBOOT / Load ELF → Decompile → Build → Run**, with **Copy to game folder** available after a successful build. The Console, Report, Roadmap, Requirements, and Controller tabs expose logs and reports. Settings are stored in `%APPDATA%/PPSX33/settings.json`.

## Requirements

Windows development uses CMake, Ninja, a C++17-capable compiler, and the .NET 8 SDK for the GUI. Visual Studio with **Desktop development with C++**, a Windows SDK, and .NET 8 is the documented development setup.

### Host requirements (run tools)

| Item | Required | Recommended |
| --- | --- | --- |
| Disk type | HDD or SSD | SSD / NVMe |
| RAM | 8 GB | 16 GB or more |
| GPU | WDDM driver (integrated OK) | Discrete, D3D11 FL 11_0 |
| VRAM | Enough for a D3D11 swap chain | 2 GB+ dedicated |
| Monitor | 1280×720 | 1920×1080 |
| Graphics API | Direct3D 11 FL 10_0+ | D3D11 FL 11_0; future OpenGL 3.3/2.1 fallback |
| OS | Windows 10 x64+ | Windows 10/11 x64 |

These are host-tool requirements. They do not imply that any game will run correctly.

## Build on Windows

```bat
scripts\build_all.bat
```

Output is placed in `build\dist\`.

## Run the synthetic smoke test

```sh
bash scripts/run_smoke_test.sh
```

On Windows, build first, then `scripts\run_smoke_test.bat`. The smoke test checks the pipeline, not commercial-game compatibility.

## Recompile workflow

1. Use an ELF you are authorized to analyze.
2. Launch `PS3Recomp.exe` → Load ELF / Decrypt EBOOT.
3. Decompile → Build → Run / Copy to game folder.
4. Successful compilation is not proof of runtime correctness.

## Repository map

```text
Compilers-files/  Compiler discovery and local tool staging guidance
docs/             Developer guide, key map, PPU coverage, game lift records
runtime/          Guest memory, split PPU helpers, logging, audio/save, SPU/RSX support
src/core/         ELF loader, PPU lifter, project/build driver
src/cli/          Command-line frontend
src/gui/          C# WinForms GUI
scripts/          Build and smoke-test scripts
roadmap/          Phase detail (XXII–XXV, …)
```

## Developer documentation and policies

- [Roadmap](ROADMAP.md) · [Phase XXV](roadmap/XXV.md)
- [Contributor workflow](CONTRIBUTING.md)
- [Developer guide](docs/DEVELOPER_GUIDE.md)
- [PPU coverage methodology](docs/PPU_COVERAGE.md)
- [Release redistributables](Redist.md)

---

*No proprietary keys or copyrighted game assets are included.*
