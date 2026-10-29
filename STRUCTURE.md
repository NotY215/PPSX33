# PPSX33 Repository Structure

**Website:** https://ppsx33.xo.je/  
**Last updated:** 2026-10-29

This document is the canonical map of the repository after the modular `src/` restructure.  
All runtime logic lives under `src/`; the old `runtime/`, `GPU/`, `Processor/`, and `Game-Stuff/` trees have been removed.

```
📦 Project_Root/
├── .gitattributes
├── .gitignore
├── CITATION.cff
├── CMakeLists.txt
├── CMakePresets.json
├── CMakeSettings.json
├── CODE_OF_CONDUCT.md
├── CONTRIBUTING.md
├── Directory.Build.props
├── ENGINEERING.md
├── LICENSE                          # Apache-2.0 (stays at root)
├── PS3Recomp.sln
├── README.md
├── ROADMAP.md
├── SECURITY.md                      # stays at root
├── STRUCTURE.md                     # this file
├── USER_MANUAL.md
├── launch.vs.json
├── requirements.md
│
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── build_error.yml
│   │   ├── config.yml
│   │   ├── game_launch.yml
│   │   ├── other.yml
│   │   ├── performance_delay.yml
│   │   └── spu_coverage.yml
│   └── workflows/
│       └── validate.yml
│
├── assets/
│   ├── PPSX33 PS3 Controller Key Mapping.png
│   ├── ppsx33-banner.png
│   ├── ppsx33-logo.ico
│   └── ppsx33-logo.png
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── DEVELOPER_GUIDE.md
│   ├── DIAGRAMS.md
│   ├── KEY_MAPPING.md
│   ├── PPU_COVERAGE.md
│   ├── legal/                       # All legal / policy docs (except LICENSE, CoC, CONTRIBUTING, SECURITY)
│   │   ├── LEGAL.md
│   │   ├── TERMS.md
│   │   ├── PRIVACY.md
│   │   ├── NOTICE
│   │   ├── THIRD_PARTY.md
│   │   ├── GOVERNANCE.md
│   │   └── Redist.md
│   └── games/                       # Per-title analysis notes & lift logs
│       ├── README.md
│       ├── DemonsSouls/
│       ├── GOW III/
│       ├── Grand Theft Auto - San Andreas/
│       ├── Grand Theft Auto V/
│       ├── Mineacraft/
│       └── Uncharted2/
│
├── redist/
│   └── README.txt
│
├── releases/
│   ├── PPSX33-Alpha-01.md
│   └── README.md
│
└── 📂 src/                          # Main Application Source Tree
    ├── 📂 cli/
    │   └── ps3_cli.cpp              # Headless CLI entry (lift / run helpers)
    │
    ├── 📂 core/                     # OS Kernel, Unencrypted ELF Loader, File Systems, HLE
    │   ├── 📂 loaders/              # Binary Executable Parsers
    │   │   ├── elf_loader.h
    │   │   └── elf_loader.cpp       # Parses unencrypted 64-bit PowerPC ELF headers, segments, symbols (no decryption)
    │   ├── 📂 fs/                   # Virtual File System (VFS) Layer
    │   │   ├── vfs.h / vfs.cpp      # Maps dev_flash, dev_hdd0, dev_bdvd to host paths
    │   │   ├── psarc_parser.h/.cpp  # Unpacks unencrypted PSARC game assets
    │   │   └── sfo_parser.h/.cpp    # Reads param.sfo metadata, titles, save profiles
    │   ├── memory.h / memory.cpp    # Main system RAM allocator (global address space)
    │   ├── hle_kernel.h / .cpp      # Intercepts native OS syscalls (threads, mutexes, file IO, modules)
    │   ├── hle_kernel_syscalls.inl  # Syscall table / dispatch bodies
    │   ├── hle_kernel_internal.h
    │   ├── lv2_syscalls.inl
    │   ├── hle_log.cpp              # Optional/reference only — ps3rt_log lives in hle_kernel.cpp (do not dual-link)
    │   ├── hle_hw_progress.cpp      # Optional/reference only — ps3rt_progress_hw lives in hle_kernel_syscalls.inl
    │   ├── hle_registry.h
    │   ├── hle_stage.h
    │   ├── hle_status.h
    │   ├── nid_table.h
    │   ├── assets.cpp
    │   ├── project.h / project.cpp  # Project / game session management
    │   ├── ps3core.h
    │   ├── ps3core_api.cpp
    │   └── ps3_util.h / ps3_util.c
    │
    ├── 📂 processor/                # Cell Broadband Engine Architecture Emulation
    │   ├── 📂 PPU/                  # PowerPC Processor Unit (Main Core)
    │   │   ├── ppu_interpreter.h/.cpp  # Interprets standard 64-bit PowerPC instructions sequentially
    │   │   ├── ppu_jit.h/.cpp          # [FUTURE] Dynamic compiler → native x86_64 / ARM64
    │   │   ├── ppu_disasm.h/.cpp       # Disassembles PPU binary segments for debug tracking
    │   │   ├── ppu_lifter.h/.cpp       # Static lift of PPU blocks to host C++ (core of recompiler)
    │   │   ├── ppu_runtime.h
    │   │   ├── ppu_runtime_core.h
    │   │   ├── ppu_runtime_core_a.h
    │   │   ├── ppu_runtime_core_b.h
    │   │   └── ppu_runtime_run.h
    │   └── 📂 SPU/                  # Synergistic Processing Units
    │       ├── spu_interpreter.h/.cpp  # Custom 128-bit SIMD ISA interpreter
    │       ├── spu_jit.h/.cpp          # [FUTURE] High-efficiency JIT for SPU loops
    │       └── spu_local_store.h/.cpp  # Isolated 256 KB Local Store model per SPU
    │
    ├── 📂 RSX/                      # Core Graphics Pipeline Emulation
    │   ├── rsx_backend_interface.h  # Pure-virtual Strategy Pattern API for backends
    │   ├── rsx_device.h/.cpp        # Decodes core libgcm commands & hardware register sets (ex-rsx_stub)
    │   ├── rsx_fifo.h/.cpp          # Ring-buffer command queue / raw GPU token parser
    │   ├── rsx_state.h/.cpp         # Tracks blend, depth, scissor, viewport, etc.
    │   ├── rsx_memory.h/.cpp        # Virtual address manager (PS3 VRAM + CPU XDR)
    │   ├── rsx_texture.h/.cpp       # Swizzle unpacker (cell formats → host formats)
    │   ├── rsx_framebuffer.h/.cpp   # PS3 render targets → modern FBOs / RTVs
    │   ├── rsx_shader_decoder.h/.cpp# Microcode binary → intermediate representation
    │   ├── rsx_shader_compiler.h/.cpp # IR → HLSL / GLSL / SPIR-V
    │   └── 📂 Backends/
    │       │   ├── rsx_dx11_backend.h/.cpp  # Context, swapchain, frame loop
    │       │   ├── rsx_dx11_state.cpp       # RSX → ID3D11Blend/Rasterizer/Depth
    │       │   ├── rsx_dx11_resources.cpp   # Buffers, layouts, textures
    │       │   └── rsx_dx11_shaders.cpp     # HLSL compile via D3DCompiler
    │       ├── 📂 OpenGL/           # [SECONDARY] OpenGL path
    │       │   ├── rsx_gl_backend.h/.cpp    # WGL/GLX context, flush, swap
    │       │   ├── rsx_gl_state.cpp         # Pipeline → glCullFace / glEnable(BLEND) …
    │       │   ├── rsx_gl_vao.cpp           # VAO + buffer layout bindings
    │       │   └── rsx_gl_shaders.cpp       # Dynamic GLSL compile/link
    │       │                                # (stubs contain “implement in future” comments)
    │       └── 📂 Vulkan/           # [DEFAULT] Vulkan 1.2 host path
    │           ├── rsx_vk_backend.h/.cpp    # VkInstance / Device / physical selection
    │           ├── rsx_vk_pipelines.cpp     # Explicit Pipeline State Objects
    │           ├── rsx_vk_command.cpp       # FIFO → VkCommandBuffer rings
    │           └── rsx_vk_memory.cpp        # Explicit VRAM sub-allocation
    │                                        # (stubs contain “implement in future” comments)
    │
    ├── 📂 games-stuff/              # Non-Graphics Subsystems & Media
    │   ├── 📂 Audio/                # Cell Audio Subsystem
    │   │   ├── 📂 Drivers/
    │   │   │   ├── audio_xaudio2_backend.cpp  # [DEFAULT] Native low-latency Windows
    │   │   │   ├── audio_sdl2_backend.cpp     # [NEXT] Portable fallback
    │   │   │   └── audio_cubeb_backend.cpp    # [FUTURE] Multi-threaded engine
    │   │   ├── 📂 Decoders/
    │   │   │   ├── decode_at3.h/.cpp          # ATRAC3plus → PCM
    │   │   │   ├── decode_adpcm.h/.cpp        # VAG / ADPCM sound effects
    │   │   │   └── decode_celp.h/.cpp         # Voice / dialogue CELP
    │   │   ├── audio_device.h/.cpp            # 48 kHz hardware clocking / port open
    │   │   ├── audio_aub.h/.cpp               # Audio Web memory & pointer loops
    │   │   ├── libaudio.h/.cpp                # cellAudio* HLE surface
    │   │   ├── audio_mixer.h/.cpp             # Multi-stream → 2.0 / 5.1 / 7.1
    │   │   ├── audio_voice.h/.cpp             # Playback state, loops, volume, pitch
    │   │   └── audio_dsp.h/.cpp               # Reverb / environmental effects
    │   └── 📂 Video/                # Cinematics & Video Playback
    │       ├── 📂 Decoders/
    │       │   ├── decode_h264.h/.cpp         # AVC / H.264 (DXVA2 path planned)
    │       │   ├── decode_mpeg2.h/.cpp        # Legacy intro clips
    │       │   └── decode_divx.h/.cpp         # DivX / commercial middleware
    │       ├── video_pamf.h/.cpp              # Sony .pam container demux
    │       ├── video_demuxer.h/.cpp           # Generic packet splitter
    │       ├── video_color_space.h/.cpp       # YUV → RGB conversion
    │       └── video_clock.h/.cpp             # Master frame pacing vs audio
    │
    └── 📂 UI/                       # User Interface, Debugger, Front-end
        ├── 📂 panels/
        │   ├── game_list_panel.h/.cpp   # VFS scan → grid/list of games
        │   ├── debugger_panel.h/.cpp    # PPU/SPU step, registers, memory view
        │   └── settings_panel.h/.cpp    # Backend / audio driver toggles
        ├── ui_manager.h/.cpp            # Main window events, init, frame pacing
        ├── ui_renderer.h/.cpp           # Basic shapes/text (ImGui-style abstraction)
        ├── MainForm.cs                  # Primary WinForms surface
        ├── MainForm.Build.cs
        ├── MainForm.Controller.cs
        ├── MainForm.Decrypt.cs
        ├── MainForm.Workflow.cs
        ├── Program.cs
        ├── Native.cs
        ├── Settings.cs
        ├── KeyMap.cs
        ├── PromptDialog.cs
        ├── TermsForm.cs
        ├── Ps3Visuals.cs
        ├── Rpcs3Launcher.cs
        ├── PS3Recomp.Gui.csproj
        └── Ui/
            ├── LiquidBackdrop.cs
            └── UiKit.cs
```

## Notes

- **Removed** (intentionally): `runtime/`, `GPU/`, `Processor/`, `Game-Stuff/`, top-level `src/` legacy copies, `tests/`, `scripts/`, duplicate decoder sources that lived outside `Decoders/`.
- **Legal** documents live under `docs/legal/` except the four root-required files: `LICENSE`, `CODE_OF_CONDUCT.md`, `CONTRIBUTING.md`, `SECURITY.md`.
- OpenGL and Vulkan backend files currently contain stub implementations with explicit “will be implemented in future” comments and a short description of each file’s intended responsibility.
- All paths in CMakeLists.txt and the GUI `.csproj` have been updated to the layout above.
