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
| ELF64 / analysis / lift | In tree; **XXV-E** unknown encodings not counted Implemented |
| Runtime | Experimental; commercial playability not established |
| RSX | Host D3D11 present + FIFO/surface depth (**XXV-D**); not full shader ISA |
| PPU run | Soft recovery with logs; **XXV-F** fault capture + optional strict mode |
| Phase XXV | **Active** — [roadmap/XXV.md](roadmap/XXV.md) |

### Phase XXV

| ID | Status | Focus |
|----|--------|--------|
| **A–E** | DONE (untested) | Audio, present, lifter honesty, RSX host depth |
| **D** | DONE (untested) | Full depth FIFO/surface/host draw |
| **F** | DONE (untested) | `validate_guest_pc`, `capture_guest_fault`, storm halt, `PS3RT_STRICT_FAULT` |
| **G–J** | PLANNED | PRX/LV2, SPU/MFC, services, E2E |

Detail: [ROADMAP.md](ROADMAP.md) · [roadmap/XXV.md](roadmap/XXV.md).

## Requirements

Windows development uses CMake, Ninja, a C++17-capable compiler, and the .NET 8 SDK for the GUI.

### Host requirements

Two tiers: **run a generated game** (output EXE + `ps3rt.dll` + game data) vs **lift/build** in the GUI.

#### Run generated game (output folder)

Derived from runtime: guest window ~512 MB address checks, default present **1280×720** D3D11 (`vs_4_0` / FL 10_0), plus Windows and game assets on disk.

| Item | Required | Recommended |
| --- | --- | --- |
| OS | Windows 10 x64 | Windows 10/11 x64 |
| Disk type | HDD (slow asset IO) | **SSD / NVMe** (PSARC / USRDIR) |
| Free disk (beyond game dump) | ~2 GB for EXE/runtime logs | 10 GB+ headroom |
| RAM | **8 GB** system (small titles / tests) | **16 GB** (GOW3-class and larger) |
| GPU | WDDM, **Direct3D 11 Feature Level 10_0** (integrated OK) | Discrete GPU, **D3D11 FL 11_0** |
| VRAM | **512 MB** usable (720p RT + guest tex) | **2 GB+** dedicated |
| Monitor | 1280×720 | 1920×1080 |
| Graphics API | D3D11 FL 10_0+ | D3D11 FL 11_0; OpenGL fallback planned |
| Audio | Shared-mode waveOut / default device | Low-latency device |
| Input | Keyboard map and/or XInput pad | XInput controller |

#### Lift / build with PPSX33 GUI (developer machine)

| Item | Required | Recommended |
| --- | --- | --- |
| RAM | 16 GB | **32 GB** (multi-million-instruction titles) |
| CPU | x64, 4 cores | 8+ cores |
| Disk | SSD | NVMe |
| Toolchain | VS C++ + Windows SDK + .NET 8 | Same |

Requirements do **not** imply that any commercial title will boot or render correctly.

## Build on Windows

```bat
scripts\build_all.bat
```

Output is placed in `build\dist\`.

## Developer documentation

- [Roadmap](ROADMAP.md) · [Phase XXV](roadmap/XXV.md)
- [Contributor workflow](CONTRIBUTING.md)
- [Developer guide](docs/DEVELOPER_GUIDE.md)

---

*No proprietary keys or copyrighted game assets are included.*
