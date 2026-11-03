# PPSX33 — Game Rendering Requirements

**Last updated:** 2026-10-31

Requirements below apply to **running a recompiled game** (the generated game EXE + `ps3rt.dll` present path).  
They do **not** describe the Visual Studio / .NET toolchain used only to **build** the recompiler GUI or lift projects.

## Supported graphics APIs (game runtime)

| API | Version | Role |
|-----|---------|------|
| **Vulkan** | **1.2** | **Default** host renderer (`vulkan-1.dll` next to the game EXE) |
| **OpenGL** | **4.3** | Secondary path (core profile); parity with Vulkan is in progress |

## Display / output

| Item | Requirement |
|------|-------------|
| Output resolution | **720×576 (576p)** fixed host present for the current path |
| Monitor | 720×576 or higher physical display recommended |
| VRAM | Enough for a 576p swapchain + staging (typically ≥ 1 GB usable) |

## Minimum (to open the game window and clear/present)

| Component | Requirement |
|-----------|-------------|
| OS | 64-bit Windows 10 or later |
| GPU | Driver that exposes **Vulkan 1.2** *or* **OpenGL 4.5** |
| Vulkan loader | `vulkan-1.dll` (copied beside the game EXE by the build) |
| Input | Keyboard / XInput pad where the title uses them |
| Game data | Lawfully obtained title files the user is authorized to run |

## Recommended (smoother present + larger FIFO/texture traffic)

| Component | Recommendation |
|-----------|----------------|
| GPU | Discrete GPU with current vendor drivers (Vulkan 1.2+) |
| VRAM | 2 GB+ |
| RAM | 8 GB+ system memory for guest + host buffers |
| Storage | SSD for game data and logs |

## What this document is not
- Not a guarantee that any title will boot or draw correctly.


## Related

- [README.md](README.md) — project overview  
- [ROADMAP.md](ROADMAP.md) — implementation status  
- [USER_MANUAL.md](USER_MANUAL.md) — GUI workflow  


## Per-game Modify.exe
API (Vulkan 1.2 / OpenGL 4.5), quality (720/560/480), and play unlock are controlled by **Modify.exe** next to the game EXE — not the main UI.
