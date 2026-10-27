# PPSX33

### *Because the Cell processor deserves a second life on the host*

> Static recompilation toolkit for PlayStation 3 titles.
> Lift decrypted ELF64 into native C++. Run with a host HLE runtime. No full emulator required.

---

## What Is This?

**PPSX33** is an experimental open-source toolkit that **statically recompiles** decrypted PlayStation 3 game binaries into native Windows executables.

Instead of interpreting or dynamically recompiling PowerPC/SPU instructions at runtime (what emulators like [RPCS3](https://github.com/RPCS3/rpcs3) do), PPSX33 takes a related but different path:

1. **Load** a decrypted ELF64 (big-endian PowerPC64).
2. **Lift** instruction regions into C++ chunks.
3. **Link** those chunks against a C/C++ **runtime** (memory, LV2/PRX HLE, SPU assist, RSX/D3D11 present).
4. **Build** a self-contained game EXE with the GUI or CLI.

This is the same broad philosophy as:

- [N64Recomp](https://github.com/N64Recomp/N64Recomp) (N64 → native)
- [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp) (Xbox 360 → native)
- [ps3recomp](https://github.com/sp00nznet/ps3recomp) (PS3 runtime libraries and tooling)
- [PS2Recomp](https://github.com/ran-j/PS2Recomp) (PS2 → native)

…applied here as an **integrated GUI + core + runtime** pipeline aimed at practical title builds (currently exercised hard against *God of War III* dumps).

PPSX33 is **not** a finished product and **does not** claim commercial playability.

## Why PS3?

The PlayStation 3 has thousands of titles and a unique architecture (Cell Broadband Engine + RSX). Emulation has proven the library can run on PC. Static recompilation tries to offer a different trade-off:

| Goal | Why it matters |
| --- | --- |
| Native host code | Lifted C++ compiles with MSVC; no per-instruction interpreter in the hot path |
| Lower runtime overhead | Once correct, work shifts to HLE accuracy, not decode bandwidth |
| Hackable ports | Generated sources and runtime sit in ordinary C++ trees |
| Focused scope | One pipeline from dump → EXE for research and preservation experiments |

## Platform support

| Platform | Status |
| --- | --- |
| **Windows x64** | Primary target (GUI, D3D11 present, MSVC game builds) |
| Linux / macOS | Not a supported host runtime today |
| Graphics API | **Direct3D 11** (Feature Level 10_0+). OpenGL fallback is planned, not shipped |
| Compiler for games | System Visual Studio C++ toolset (via `vswhere`). `redist/` is installer + guide only |

## The Challenge

PS3 software is not a single CPU blob. A serious port has to confront:

| Piece | What it is | Why it is hard |
| --- | --- | --- |
| **PPU** | PowerPC 64-bit main CPU | BE memory, CR/LR/CTR, VMX/AltiVec |
| **SPUs** | 6× synergistic processors | Local store, MFC DMA, SPURS |
| **RSX** | NV40-derived GPU | GCM FIFO, surfaces, textures, shaders |
| **LV2** | Kernel syscalls | Threads, events, memory, time, FS |
| **PRX** | Dynamic modules | NID exports, sysmodule load order |

PPSX33 attacks this with a **lifter + HLE runtime**, not a cycle-accurate emulator. Accuracy grows subsystem by subsystem. See [ROADMAP.md](ROADMAP.md).

## Architecture

```
PPSX33/
├── src/
│   ├── core/                 # ELF load, PPU lifter, project/build orchestration
│   ├── cli/                  # Headless tools
│   └── gui/                  # WinForms app (load, decompile, controller map, build)
├── runtime/                  # Linked into every generated game EXE / ps3rt.dll
│   ├── ppu_runtime*.h        # Guest context, run loop, recovery
│   ├── ps3rt*.cpp / *.inl    # HLE, GCM, PRX, syscalls
│   ├── rsx_stub.cpp          # RSX FIFO → D3D11 present
│   ├── spu_stub.cpp          # SPU / MFC assist
│   ├── assets.cpp            # Package / path helpers
│   └── audio_save.cpp        # Audio + save HLE
├── scripts/                  # Host build helpers
├── redist/                   # VS installer pointer + setup guide (not a full compiler tree)
├── docs/                     # Notes, game logs (e.g. GOW III)
├── ROADMAP.md                # Implementation status (single file)
└── README.md                 # This file
```

**Generated game output** (per project) typically contains:

```
projects/<Title>/output/
├── <Title>.exe
├── ps3rt.dll
├── guest_image.bin
├── pad_map.json
├── src/                      # Lifted chunks + runtime copies used by the game build
└── build_live.log
```

## How It Works

### 1. Load

You point the GUI at a **decrypted** ELF / game layout you are allowed to use. The core maps segments into a guest address space and records entry / TOC-related data needed for the lift.

### 2. Decompile / lift

The PPU lifter walks code regions and emits C++ **chunks**. Coverage is title-dependent: large games still show non-trivial unimplemented opcode counts in lift reports. Unimplemented paths must not be silently treated as “correct.”

### 3. Runtime HLE

At run time, the EXE does not emulate a full LV2. It:

- Executes lifted PPU code against a register/memory context
- Services syscalls and PRX imports through **HLE tables** (`nid_table.h`, LV2 cases, module stubs)
- Advances SPU-related work at a coarse level where implemented
- Drives **RSX** by decoding GCM FIFO methods into host D3D11 draws and presents

### 4. Graphics path (current)

1. Game or runtime initializes cellGcm-style control/labels/local memory.
2. FIFO **PUT/GET** are normalized; JUMP/CALL decoding follows NV40-style rules (false JUMP fixed).
3. Methods update surface, clear, texture, and vertex-array state.
4. **DRAW_ARRAYS** / **DRAW_INDEX_ARRAY** issue host draws when vertex data is available.
5. Present blits the guest color surface (and diagnostic geometry only when the title has not submitted real work).

**Evidence from GOW III logs:** the host window, CLEAR, inject triangle, and stable method counts work. **Title-authored meshes and shaders are not complete yet** while boot/import stalls limit real GCM PUT traffic.

### 5. Build

The GUI writes sources and invokes the **system** MSVC toolchain (`vswhere` → `vcvars64` → `cl`/`link`). Successful builds produce the game EXE next to `ps3rt.dll` and the guest image.

## Implementation status

Full tables live in **[ROADMAP.md](ROADMAP.md)**. Summary:

| Area | Status |
| --- | --- |
| ELF load / GUI / MSVC game build | **Done** (title-dependent lift coverage) |
| D3D11 window + present | **Done** |
| GCM FIFO decode (JUMP/PUT/GET) | **Done** |
| Surface bind + solid CLEAR | **Done** |
| Seed / inject geometry | **Partial** (diagnostic until title GCM runs) |
| Real game meshes / textures / SCE shaders | **In progress** |
| LV2 / PRX / boot completeness | **Partial** |
| SPU / MFC / SPURS | **Partial** |
| Audio / pad / save | **Partial** |

## Requirements

### Run a generated game

| Item | Required | Recommended |
| --- | --- | --- |
| OS | Windows 10 x64 | Windows 11 x64 |
| RAM | 8 GB | 16 GB |
| GPU | D3D11 Feature Level **10_0** (integrated OK) | Discrete, FL **11_0** |
| VRAM | ~512 MB usable at 720p | 2 GB+ |
| Display | 1280×720 | 1920×1080 |
| Disk | Game dump + a few GB free | SSD/NVMe |
| Input | Keyboard map and/or XInput | XInput controller |
| Audio | Default shared-mode device | Low-latency device |

### Lift and build with PPSX33

| Item | Required | Recommended |
| --- | --- | --- |
| Toolchain | Visual Studio C++ tools + Windows SDK | VS 2022/2026 Community or Build Tools |
| .NET | SDK **8+** (GUI) | Same |
| CMake / Ninja | For the host `ps3core` / CLI tree | Same |
| RAM | 16 GB | 32 GB |
| Disk | SSD | NVMe |

`redist/README.txt` documents which VS packages to install. The app does **not** ship a full compiler tree; it uses the system toolchain.

## Getting Started

1. Install Visual Studio C++ workload and .NET 8 SDK.
2. Clone this repository.
3. Build the host tools:

```bat
scripts\build_all.bat
```

4. Run `build\dist\PS3Recomp.exe`.
5. Accept the legal notice, load a **decrypted** ELF / project you are allowed to use.
6. Decompile → configure controller map → Build.
7. Run the EXE under `projects\<name>\output\`.

Expect logs under the game folder and in the GUI console. For GOW III experiments, compare against notes under `docs/games/`.

## Relationship to other projects

| Project | Relationship |
| --- | --- |
| [RPCS3](https://github.com/RPCS3/rpcs3) | Full emulator; PPSX33 uses public RSX/GCM knowledge, not RPCS3 as a runtime |
| [ps3recomp](https://github.com/sp00nznet/ps3recomp) | Kindred static-recomp / runtime effort; different tree and goals |
| N64Recomp / UnleashedRecomp | Same *idea* (ahead-of-time lift), different ISA and OS |

PPSX33 does not redistribute game assets, firmware, or encrypted SELF content.

## Documentation

| Doc | Contents |
| --- | --- |
| [ROADMAP.md](ROADMAP.md) | Implementation-level Done / Partial / In progress |
| `docs/` | Game notes and runtime logs |
| `runtime/README.md` | Runtime module notes (if present) |
| `redist/README.txt` | VS install and path setup |

## Contributing

Useful contributions:

- PPU opcode coverage with tests or lift-report before/after
- LV2/PRX behavior grounded in real title logs
- RSX method handling with FIFO traces (not diagnostic-only paths)
- Build script and GUI reliability on clean Windows machines

Please keep changes focused, avoid drive-by refactors, and do not commit copyrighted game binaries.

## Legal

Only use game dumps, firmware, and keys you have the right to use under the laws that apply to you. This project is for interoperability research and personal preservation experiments. Authors are not responsible for misuse.

## License

See repository license files if present. Third-party headers and system SDKs remain under their own terms.
