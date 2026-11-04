# PPSX33 Engineering notes

**Website:** [https://ppsx33.xo.je/](https://ppsx33.xo.je/)  
**Architecture:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)  
**Roadmap:** [ROADMAP.md](ROADMAP.md)  
**Structure:** [STRUCTURE.md](STRUCTURE.md)

## Host build

- CMake, Windows x64, **MSVC**, **Qt 6** Widgets (`find_package(Qt6 REQUIRED COMPONENTS Widgets)`).
- Qt install example: `aqt install-qt windows desktop 6.8.0 win64_msvc2022_64 -O C:\Qt` then add `C:\Qt\6.8.0\msvc2022_64\bin` to PATH.
- Output: `build/dist/PPSX33.exe`, `ps3core.dll`, **`ps3rt.dll`** (unified), `vulkan-1.dll`, logos, `runtime/` headers.
- Host version: **1.1.0-Alpha** (RC VERSIONINFO + `AppVersion.h`). `PPSX33.exe -version`

## Runtime model

Single **`ps3rt.dll`** contains HLE, RSX FIFO/device, Vulkan backend, present/draw, audio, shader bind. Game EXE links `ps3rt.lib` and loads `ps3rt.dll`.

## Game build path

1. Load **decrypted ELF** (legal/terms gate).
2. Decompile / lift → project `codebase/` then `output/src`.
3. MSVC `cl` / `link` via `vcvars64`; prefer prebuilt `ps3rt.dll` + `ps3rt.lib` + `runtime/*.h`.
4. Game EXE embeds the same **ppsx33-logo.ico** when present next to dist/output.

## RSX / present

- Control: PUT@0, REF@4, GET@8 (BE).
- Flip → display buffer offset → guest RGBA present or clear policy.
- Shader: `rsx_shader_bind_*` tracks VP/FP; host program body / SPIR-V still thin.

## Correctness env

| Variable | Effect |
|----------|--------|
| `PS3RT_STRICT_FAULT=1` | Halt on invalid PC |
| `PS3RT_INJECT_SEED=0` | Disable boot FIFO assist |

## Logging

Runtime logs: `GameName-YYYY-MM-DD_HH-MM-SS.txt` under USRDIR / Report tab. Assist draws and soft recovery are **diagnostic**.

## Graphics backends

| Backend | Status |
|---------|--------|
| Vulkan 1.1 | Default host present (swapchain + guest RGBA staging) |
| OpenGL (WGL) | Secondary; working-phase; fixed-function textured quad present |

Set via UI combo or `ps3rt_set_graphics_backend` (0=Vulkan, 1=OpenGL).
