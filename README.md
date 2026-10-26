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
| ELF64 / analysis / lift | In tree; large titles still report many unimplemented encodings |
| Runtime | Experimental; commercial playability **not** established |
| RSX host present | D3D11 window, FIFO decode, surface bind, **seed** frame (color bars + triangle) |
| RSX game geometry | **Not started** as title output; GCM PUT stays stalled on GOW3 logs |
| PPU run | Soft recovery, sticky BREAK, import stubs; many unknown NIDs |
| Phase focus | **XV** real rendering path; see [roadmap/XV.md](roadmap/XV.md) |

### What works today (evidence from GOW3 run logs)

- Guest image load, PSARC scan, D3D11 host window titled `PPSX33 presents GOW3`
- Seed present: rainbow bars + large triangle (inject + guest color paint)
- Flip counter and draw counters advance

### What does **not** work yet (same evidence)

- Real game meshes, textures, or menus from GOW3 command streams
- GCM FIFO often idle (`put == get`); inject path supplies seed draws
- PRX import path previously accepted **allocation sizes** as NIDs (fixed in XV-J)
- Sticky wild-PC recovery can still loop; HARD escape added in XV-J

### Phase XV (rendering)

| ID | Status | Focus |
|----|--------|--------|
| A-H | Done / in tree | Metrics, FIFO jump, present order, textures, depth/blend, stable seed |
| **I** | In tree | One seed composition per flip; inject throttle; per-frame counters |
| **J** | In tree (this update) | NID filter (reject sizes), sticky HARD escape |
| **K+** | Next | Real GCM from title after boot/import advances |

Detail: [ROADMAP.md](ROADMAP.md) · [roadmap/XV.md](roadmap/XV.md).

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
