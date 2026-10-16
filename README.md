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
| PPU static lifting | High instruction counts on sample titles; **counts are not semantic correctness** (see XXV-C/E) |
| PPU semantics | Partial; audit-driven honesty work in Phase XXV |
| Project workflow | GUI and CLI support project creation, lifting, and host build invocation |
| Windows output | Build path can emit game EXE, `ps3rt.dll`, and `guest_image.bin` |
| Runtime correctness | Experimental; commercial-game execution is not established |
| RSX | D3D11 present + partial FIFO; **not** faithful game rendering yet (XXV-D) |
| Phase XXII | Finished — [roadmap/XXII.md](roadmap/XXII.md) |
| Phase XXIII | In tree — [roadmap/XXIII.md](roadmap/XXIII.md) |
| Phase XXIV | Boot-to-first-pixels — [roadmap/XXIV.md](roadmap/XXIV.md) |
| Phase XXV | **Active** — truthful exec & pixels — [roadmap/XXV.md](roadmap/XXV.md) |

## Current runtime development status

The current [ROADMAP.md](ROADMAP.md) tracks tooling (**I–XIX**), packaging (**XX**), lettered **XXI**, pipeline **XXII** (finished), execution reality **XXIII**, boot assist **XXIV**, and active **XXV** (truthful PPU/RSX/HLE per audit). Milestone labels describe engineering work, not verified commercial-title compatibility.

### Phase XXV (active) — expanded

| ID | Focus |
|----|--------|
| **A–B** | Audio host build fix; visible D3D11 present |
| **C / E** | Lifter honesty → full status + opcode audit |
| **D** | RSX FIFO decode, guest color surface, real present |
| **F** | Strict guest faults (`validate_guest_pc`, `capture_guest_fault`) |
| **G** | PRX/LV2 API contracts (no silent success) |
| **H** | SPU / MFC / SPURS layered correctness |
| **I–J** | FS/audio/pad/save services; end-to-end title proof |

Approach (audit-aligned): capture failing PC/NID/method → define semantics → implement real behaviour → re-test. Detail and function checklist: [roadmap/XXV.md](roadmap/XXV.md).

The PPU runtime is split across `runtime/ppu_runtime.h`, `runtime/ppu_runtime_core.h`, `runtime/ppu_runtime_core_a.h`, `runtime/ppu_runtime_core_b.h`, and `runtime/ppu_runtime_run.h`. Additional playability code lives in `runtime/ps3rt.cpp`, `runtime/ps3rt_b.inl`, `runtime/spu_stub.cpp`, `runtime/assets.cpp`, `runtime/audio_save.cpp`, and `runtime/rsx_stub.cpp`.

## Project website and releases

- **Project website:** [notyvos.gt.tc/PPSX3](https://notyvos.gt.tc/PPSX3/)
- **Release documentation:** [releases/README.md](releases/README.md)
- **Alpha 01 notes:** [releases/PPSX33-Alpha-01.md](releases/PPSX33-Alpha-01.md)
- **Release prerequisites:** [Redist.md](Redist.md)

## GUI status and settings

The GUI workflow is **Decrypt EBOOT / Load ELF → Decompile → Build → Run**, with **Copy to game folder** available after a successful build. The Console, Report, Roadmap, Requirements, and Controller tabs expose logs and reports.

Settings are stored in `%APPDATA%/PPSX33/settings.json`.

## Requirements

Windows development uses CMake, Ninja, a C++17-capable compiler, and the .NET 8 SDK for the GUI.

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

## Developer documentation

- [Roadmap](ROADMAP.md)
- [Phase XXV detail](roadmap/XXV.md)
- [Contributor workflow](CONTRIBUTING.md)
- [Developer guide](docs/DEVELOPER_GUIDE.md)
- [PPU coverage methodology](docs/PPU_COVERAGE.md)

---

*No proprietary keys or copyrighted game assets are included.*
