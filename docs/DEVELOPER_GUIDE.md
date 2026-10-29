# PPSX33 Developer Guide

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="120" /></p>


This guide describes the repository layout, development workflow, and implementation responsibilities for contributors.

## Architecture overview

| Component | Location | Responsibility |
| --- | --- | --- |
| ELF loader | `src/core/loaders/elf_loader.cpp`, `src/core/loaders/elf_loader.h` | Parse ELF64 big-endian PowerPC64 inputs, loadable segments, entry points, symbols, and function descriptors |
| PPU lifter | `src/processor/PPU/ppu_lifter.cpp`, `src/processor/PPU/ppu_lifter.h` | Decode guest PPU instructions and emit translated C/C++ code |
| Project/build driver | `src/core/project.cpp`, `src/core/project.h` | Manage project files, invoke lifting, and coordinate host builds |
| Core API | `src/core/ps3core.h`, `src/core/ps3core_api.cpp` | Public native interface consumed by frontends |
| Runtime | `src/processor/PPU/ppu_runtime.h`, `src/core/hle_kernel.cpp` | Guest memory, CPU state helpers, log, and HLE runtime |
| SPU runtime | `src/processor/SPU/spu_interpreter.cpp` | SPU context, ISA step, mailboxes, and MFC; SPURS coverage remains partial |
| RSX runtime | `src/RSX/rsx_device.cpp`, `src/RSX/rsx_fifo.cpp` | FIFO/method processing and D3D11 present; not a complete RSX implementation |
| CLI | `src/cli/ps3_cli.cpp` | Command-line entry point for project processing |
| GUI | `src/UI/` | C# WinForms interface and native interop |

## Build environment

Windows development uses Visual Studio with the Desktop development with C++ workload and a Windows SDK, plus CMake, Ninja, and the .NET 8 SDK.

**ps3rt link units:** do not dual-link `hle_log.cpp` or `hle_hw_progress.cpp` with `hle_kernel.cpp` (LNK2005 on `ps3rt_log` / `ps3rt_progress_hw`). Logging and `ps3rt_progress_hw` are owned by `hle_kernel.cpp` + `hle_kernel_syscalls.inl`.

## Phase XX runtime modules

| Module | Path | Role |
|--------|------|------|
| PPU loop / null-loop | `src/processor/PPU/ppu_runtime_run.h` | Soft-break / region skip (XX-H) |
| GCM + SPURS HLE | `src/core/hle_kernel_syscalls.inl`, `src/core/nid_table.h` | `gcm_hle`, `spurs_dispatch` (XX-I, XX-M) |
| Assets / PSARC | `src/core/assets.cpp` | cellFs + zlib block streaming (XX-J) |
| SPU interpreter | `src/processor/SPU/spu_interpreter.cpp` | ISA step, MFC, SPURS schedule (XX-K–M) |
| Runtime shell | `src/core/hle_kernel.cpp` | Lifecycle, log, pad, memory; progress_hw in syscalls.inl |
| Context | `src/processor/PPU/ppu_runtime_core_a.h` | `SPUContext` MFC/SPURS fields |
| LV2 extras | `src/core/lv2_syscalls.inl` | `sys_spu_thread_*` range |
| Audio / save | `src/games-stuff/Audio/audio_device.cpp` | cellAudio → XAudio2/waveOut (XX-N) |
| RSX host | `src/RSX/rsx_device.cpp` | Present, FIFO, D3D11 (XX-N) |

### Host dependencies (Windows)

- **MSVC** toolset as used by the GUI build path  
- **zlib1.dll** (or `zlib.dll`) next to the game EXE for compressed PSARC blocks  
- **xaudio2** / **winmm** linked for audio; **d3d11** for present  
