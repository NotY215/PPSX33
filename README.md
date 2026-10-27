# PPSX33

### PlayStation 3 static recompiler

Decode · Translate · Native host runtime

PPSX33 is an experimental toolkit that lifts decrypted PS3 ELF64 (big-endian PowerPC64) into host-native code and runs it with a C/C++ runtime (HLE, SPU assist, D3D11 present). It is **not** a complete PS3 emulator and does **not** claim commercial playability.

Inspired by the static-recomp approach used in projects such as [N64Recomp](https://github.com/N64Recomp/N64Recomp) and the runtime/library focus of [ps3recomp](https://github.com/sp00nznet/ps3recomp).

## Status (evidence from GOW3 logs)

| Area | State |
| --- | --- |
| ELF load / lift / GUI build | **Done** (title-dependent coverage) |
| Host window + D3D11 present | **Done** |
| GCM FIFO decode (JUMP fix, PUT/GET) | **Done** |
| CLEAR + surface bind | **Done** |
| Seed geometry (inject triangle) | **Partial** (diagnostic until title GCM runs) |
| Real game meshes / textures / shaders | **In progress** |
| Boot / LV2 / PRX completeness | **Partial** |
| Audio / pad / save | **Partial** |

See [ROADMAP.md](ROADMAP.md) for implementation-level detail.

## What works today

- Load decrypted ELF, decompile/lift chunks, MSVC game build via GUI
- Guest memory, basic LV2 alloc/time/thread stubs, cellGcm init path
- D3D11 window titled `PPSX33 presents <game>`
- FIFO methods without the old false-JUMP storm (`methods≈22` / inject packet counts)
- Solid CLEAR into guest color; host blit of that surface

## What does not work yet

- God of War III (and similar) full scenes: title GCM PUT still mostly idle; inject supplies the visible triangle
- Full SCE VP/FP shader ISA, accurate SPU/MFC, complete PRX set
- Sticky wild-PC recovery can still loop in some boot regions

## Requirements

### Run a generated game

| Item | Required | Recommended |
| --- | --- | --- |
| OS | Windows 10 x64 | Windows 11 x64 |
| RAM | 8 GB | 16 GB |
| GPU | D3D11 Feature Level 10_0 | FL 11_0 discrete |
| VRAM | 512 MB usable | 2 GB+ |
| Disk | HDD + game dump | SSD/NVMe |
| Display | 1280×720 | 1080p |

### Build / lift with the GUI

| Item | Required | Recommended |
| --- | --- | --- |
| Toolchain | VS C++ tools, Windows SDK, .NET 8 | Same |
| RAM | 16 GB | 32 GB |
| Disk | SSD | NVMe |

Install Visual Studio Build Tools (or Community) with the C++ workload. The `redist/` folder is the installer guide only; the host build uses the system compiler.

## Build

```bat
scripts\build_all.bat
```

Output under `build\dist\`.

## Legal

Only use dumps and firmware you are legally allowed to use. This project does not distribute game assets or official firmware.

## Docs

- [ROADMAP.md](ROADMAP.md) — implementation status (single file)
- [TERMS.md](TERMS.md) / [LEGAL.md](LEGAL.md) if present in the tree
