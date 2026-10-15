# PPSX33 Host Requirements

**Last updated:** 2026-10-15

This document describes the host PC requirements for building and running the experimental PPSX33 static recompiler. Meeting these requirements does **not** guarantee that any particular PS3 title will boot, render correctly, or be playable.

## Minimum target

| Component | Requirement |
|---|---|
| Operating system | 64-bit Windows 10 or later |
| CPU | 64-bit x86 processor supported by the required Windows and MSVC toolchain |
| Memory | 8 GB RAM |
| Storage | HDD or SSD; SSD recommended for builds and game data |
| Graphics | GPU with a working WDDM driver and Direct3D 11 support, Feature Level 10_0 or higher for the current graphics path |
| Video memory | Dedicated or shared GPU memory sufficient for a D3D11 swap chain; clear/present-only operation may work on modest integrated graphics |
| Display | 1280×720 or higher recommended |
| Game input | Keyboard mapping; XInput-compatible controller support where available |
| Input files | A legally obtained, decrypted PS3 ELF/EBOOT that the user is authorized to use; encrypted SELF loading is not supported by the current documented path |

## Recommended target

| Component | Recommendation |
|---|---|
| Operating system | 64-bit Windows 10 or later, fully updated |
| Memory | 16 GB RAM or more |
| Storage | SSD or NVMe SSD with enough free space for source, generated projects, logs, and game data |
| Graphics | Discrete GPU with Direct3D 11 Feature Level 11_0 or better |
| Video memory | 2 GB dedicated VRAM or more |
| Display | 1920×1080 or higher |

## Build tools

- Use a compatible Visual Studio / MSVC C++ toolchain and the Windows SDK required by the solution and build scripts.
- Run `scripts\\build_all.bat` from a suitable Windows development environment.
- Consult [USER_MANUAL.md](USER_MANUAL.md) and [ROADMAP.md](ROADMAP.md) for workflow and current implementation status.

## Important limitations

- These are practical host targets, not measured guarantees or strict pass/fail gates.
- Graphics support remains incomplete. A system that can open the D3D11 window may still show a clear color, black screen, or incorrect rendering.
- PPU/SPU instruction coverage, syscall and HLE behavior, filesystem support, audio, timing, and PRX loading remain title-dependent and incomplete.
- OpenGL fallback is a future direction, not a currently guaranteed alternative to D3D11.
- Supply your own legally obtained game files. PPSX33 does not include proprietary decryption keys or copyrighted game assets.

## Related documentation

- [README.md](README.md)
- [ROADMAP.md](ROADMAP.md)
- [USER_MANUAL.md](USER_MANUAL.md)
