# PPSX33 Repository Structure

**Website:** https://ppsx33.xo.je/  
**Repository:** https://github.com/NotY215/PPSX33  
**Last updated:** 2026-11-02

This document describes the tracked layout of the current PPSX33 repository, based on the `main` branch. It includes the project configuration, documentation, assets, native runtime, Qt desktop UI, graphics pipeline, game-analysis records, and tests. File roles describe their apparent responsibility from their names and project integration; they do not imply that every subsystem is complete or that every game is playable.

## Repository map

```text
PPSX33/
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── build_error.yml
│   │   ├── config.yml
│   │   ├── game_launch.yml
│   │   ├── other.yml
│   │   ├── performance_delay.yml
│   │   └── spu_coverage.yml
│   └── workflows/
│       └── validate.yml                 # Repository validation workflow
├── assets/
│   ├── PPSX33 PS3 Controller Key Mapping.png
│   ├── ppsx33-banner.png
│   ├── ppsx33-logo.ico                  # Windows application icon/resource
│   └── ppsx33-logo.png                  # Documentation and UI logo
├── cmake/
│   └── CopyVulkanRuntime.cmake          # Stages Vulkan runtime beside built outputs
├── docs/
│   ├── ARCHITECTURE.md                  # Runtime and system architecture
│   ├── DEVELOPER_GUIDE.md               # Developer setup and workflow
│   ├── DIAGRAMS.md                      # Architecture diagram sources and index
│   ├── KEY_MAPPING.md                   # Keyboard/controller mapping reference
│   ├── PPU_COVERAGE.md                  # PPU implementation and coverage notes
│   ├── games/
│   │   ├── README.md                    # Game-analysis index and conventions
│   │   ├── DemonsSouls/
│   │   │   ├── DEMONS_SOULS.md
│   │   │   ├── DemonsSouls_lift_report.txt
│   │   │   ├── README.md
│   │   │   └── analysis_report.txt
│   │   ├── GOW III/
│   │   │   ├── GOW3-2026-11-02_13-33-15.txt
│   │   │   ├── GOW3.md
│   │   │   ├── README.md
│   │   │   ├── analysis_report.txt
│   │   │   └── lift_report.txt
│   │   ├── Grand Theft Auto - San Andreas/
│   │   │   ├── Grand Theft Auto - San Andreas.md
│   │   │   ├── README.md
│   │   │   ├── analysis_report.txt
│   │   │   └── lift_report.txt
│   │   ├── Grand Theft Auto V/
│   │   │   ├── Grand Theft Auto V.md
│   │   │   ├── README.md
│   │   │   ├── analysis_report.txt
│   │   │   └── lift_report.txt
│   │   ├── Mineacraft/
│   │   │   ├── MINECRAFT.md
│   │   │   ├── README.md
│   │   │   ├── analysis_report.txt
│   │   │   └── lift_report.txt
│   │   └── Uncharted2/
│   │       ├── README.md
│   │       ├── UNCHARTED2.md
│   │       ├── UNCHARTED2_lift_report.txt
│   │       └── analysis_report.txt
│   └── legal/
│       ├── GOVERNANCE.md                # Project governance
│       ├── LEGAL.md                     # Legal overview
│       ├── NOTICE                       # Notices
│       ├── PRIVACY.md                   # Privacy information
│       ├── Redist.md                    # Redistribution notes
│       ├── TERMS.md                     # Terms
│       └── THIRD_PARTY.md               # Third-party components and notices
├── redist/
│   └── README.txt                       # Runtime/redistribution staging notes
├── releases/
│   ├── PPSX33-Alpha-01.md
│   ├── PPSX33-Alpha-1.0.4.md
│   └── README.md                        # Release-note index
├── src/
│   ├── RSX/                             # RSX graphics device and command pipeline
│   │   ├── Backends/
│   │   │   ├── OpenGL/
│   │   │   │   ├── rsx_gl_backend.cpp   # OpenGL backend setup/submission
│   │   │   │   ├── rsx_gl_backend.h
│   │   │   │   ├── rsx_gl_shaders.cpp   # GLSL shader helpers
│   │   │   │   ├── rsx_gl_state.cpp     # OpenGL state handling
│   │   │   │   └── rsx_gl_vao.cpp       # Vertex-array/buffer binding
│   │   │   └── Vulkan/
│   │   │       ├── rsx_vk_backend.cpp   # Vulkan backend and device work
│   │   │       ├── rsx_vk_backend.h
│   │   │       ├── rsx_vk_command.cpp   # Command-buffer submission
│   │   │       ├── rsx_vk_memory.cpp    # Vulkan memory helpers
│   │   │       └── rsx_vk_pipelines.cpp # Graphics pipeline setup
│   │   ├── dispatch/
│   │   │   ├── rsx_dispatch_sink.c      # Dispatch sink for decoded methods
│   │   │   └── rsx_dispatch_sink.h
│   │   ├── draw/
│   │   │   ├── rsx_draw_batch.c         # Draw batching
│   │   │   ├── rsx_draw_batch.h
│   │   │   ├── rsx_surface_fmt.c        # Surface format utilities
│   │   │   └── rsx_surface_fmt.h
│   │   ├── fifo/
│   │   │   ├── rsx_fifo_drain.c         # FIFO draining/command consumption
│   │   │   ├── rsx_fifo_drain.h
│   │   │   ├── rsx_method_table.c       # RSX method classification/dispatch table
│   │   │   └── rsx_method_table.h
│   │   ├── methods/
│   │   │   └── rsx_nv4097_surface.c     # NV4097 surface-method handling
│   │   ├── present/
│   │   │   ├── rsx_present_policy.c     # Presentation policy
│   │   │   ├── rsx_present_policy.h
│   │   │   ├── rsx_present_queue.c      # Frame/present queue
│   │   │   └── rsx_present_queue.h
│   │   ├── render/
│   │   │   ├── rsx_light_present.c      # Lightweight presentation path
│   │   │   ├── rsx_light_present.h
│   │   │   ├── rsx_render_frame.c       # Frame orchestration
│   │   │   └── rsx_render_frame.h
│   │   ├── resolution/
│   │   │   ├── rsx_resolution.c         # Resolution selection/helpers
│   │   │   └── rsx_resolution.h
│   │   ├── rsx_backend_interface.h      # Shared backend contract
│   │   ├── rsx_device.cpp               # RSX device/register handling
│   │   ├── rsx_device.h
│   │   ├── rsx_exports.c                # Runtime exports
│   │   ├── rsx_fifo.cpp                 # FIFO integration
│   │   ├── rsx_fifo.h
│   │   ├── rsx_fifo_guest_put_notes.h   # Guest PUT/FIFO diagnostic notes
│   │   ├── rsx_framebuffer.cpp          # Framebuffer/render-target management
│   │   ├── rsx_framebuffer.h
│   │   ├── rsx_memory.cpp               # RSX guest memory
│   │   ├── rsx_memory.h
│   │   ├── rsx_shader_bind.c            # Shader binding state and readiness
│   │   ├── rsx_shader_bind.h
│   │   ├── rsx_shader_compiler.cpp      # Host shader compilation path
│   │   ├── rsx_shader_compiler.h
│   │   ├── rsx_shader_decoder.cpp       # RSX shader decoding
│   │   ├── rsx_shader_decoder.h
│   │   ├── rsx_state.cpp                # Graphics state tracking
│   │   ├── rsx_state.h
│   │   ├── rsx_texture.cpp              # Texture layout/format handling
│   │   └── rsx_texture.h
│   ├── cli/
│   │   └── ps3_cli.cpp                  # Command-line entry point/source
│   ├── core/                            # Guest system, loaders, memory, and HLE
│   │   ├── fs/
│   │   │   ├── psarc_parser.cpp         # PSARC archive parsing
│   │   │   ├── psarc_parser.h
│   │   │   ├── sfo_parser.cpp           # PARAM.SFO parsing
│   │   │   ├── sfo_parser.h
│   │   │   ├── vfs.cpp                  # Guest-to-host virtual filesystem
│   │   │   └── vfs.h
│   │   ├── hle/
│   │   │   ├── gcm/
│   │   │   │   ├── gcm_control_bridge.c # GCM control bridge
│   │   │   │   ├── gcm_flip_label.c    # Flip-label handling
│   │   │   │   └── gcm_flip_label.h
│   │   │   ├── spurs_stub.h             # SPURS stub declarations
│   │   │   ├── sysmodule_progress.c     # Sysmodule progress/status helpers
│   │   │   └── sysmodule_progress.h
│   │   ├── import/
│   │   │   ├── import_stub_map.c        # Guest import/NID stub map
│   │   │   └── import_stub_map.h
│   │   ├── loaders/
│   │   │   ├── elf_loader.cpp           # ELF image loading
│   │   │   └── elf_loader.h
│   │   ├── stats/
│   │   │   ├── host_stats.c             # Host/runtime statistics
│   │   │   └── host_stats.h
│   │   ├── assets.cpp                   # Core asset helpers
│   │   ├── hle_hw_progress.cpp          # Hardware progress/reference helpers
│   │   ├── hle_kernel.cpp               # Kernel HLE/syscall implementation
│   │   ├── hle_kernel.h
│   │   ├── hle_kernel_internal.h
│   │   ├── hle_kernel_syscalls.inl      # Kernel syscall implementation include
│   │   ├── hle_log.cpp                  # HLE logging helpers
│   │   ├── hle_registry.h               # HLE registration declarations
│   │   ├── hle_stage.h                  # HLE startup-stage definitions
│   │   ├── hle_status.h                 # HLE status definitions
│   │   ├── lv2_syscalls.inl             # LV2 syscall definitions
│   │   ├── memory.cpp                   # Guest memory management
│   │   ├── memory.h
│   │   ├── nid_table.h                  # NID lookup definitions
│   │   ├── project.cpp                  # Project/game-session logic
│   │   ├── project.h
│   │   ├── ps3_util.c                   # Shared low-level PS3 helpers
│   │   ├── ps3_util.h
│   │   ├── ps3core.h                    # Core API/shared types
│   │   └── ps3core_api.cpp              # Core API implementation
│   ├── games-stuff/                     # Game-facing media support
│   │   ├── Audio/
│   │   │   ├── Decoders/
│   │   │   │   ├── decode_adpcm.cpp     # ADPCM/VAG-style audio decoder
│   │   │   │   ├── decode_adpcm.h
│   │   │   │   ├── decode_at3.cpp       # ATRAC-family decoder
│   │   │   │   ├── decode_at3.h
│   │   │   │   ├── decode_celp.cpp      # CELP decoder
│   │   │   │   └── decode_celp.h
│   │   │   ├── Drivers/
│   │   │   │   ├── audio_cubeb_backend.cpp # cubeb output backend
│   │   │   │   ├── audio_sdl2_backend.cpp  # SDL2 output backend
│   │   │   │   └── audio_xaudio2_backend.cpp # Windows XAudio2 output backend
│   │   │   ├── audio_aub.cpp            # Audio buffer helpers
│   │   │   ├── audio_aub.h
│   │   │   ├── audio_device.cpp         # Host audio device lifecycle
│   │   │   ├── audio_device.h
│   │   │   ├── audio_dsp.cpp            # DSP/effects helpers
│   │   │   ├── audio_dsp.h
│   │   │   ├── audio_mixer.cpp          # Audio stream mixing
│   │   │   ├── audio_mixer.h
│   │   │   ├── audio_voice.cpp          # Voice playback state
│   │   │   ├── audio_voice.h
│   │   │   ├── libaudio.cpp             # cellAudio-facing HLE implementation
│   │   │   └── libaudio.h
│   │   └── Video/
│   │       ├── Decoders/
│   │       │   ├── decode_divx.cpp      # DivX decoding path
│   │       │   ├── decode_divx.h
│   │       │   ├── decode_h264.cpp      # H.264/AVC decoding path
│   │       │   ├── decode_h264.h
│   │       │   ├── decode_mpeg2.cpp     # MPEG-2 decoding path
│   │       │   └── decode_mpeg2.h
│   │       ├── video_clock.cpp          # Video timing/frame pacing
│   │       ├── video_clock.h
│   │       ├── video_color_space.cpp    # Color-space conversion
│   │       ├── video_color_space.h
│   │       ├── video_demuxer.cpp        # Video packet demultiplexing
│   │       ├── video_demuxer.h
│   │       ├── video_pamf.cpp           # PAMF container handling
│   │       ├── video_pamf.h
│   │       ├── video_presenter.cpp      # Decoded-frame presentation
│   │       └── video_presenter.h
│   ├── native/
│   │   └── x64/
│   │       ├── ps3rt_be.c               # Big-endian load/store helpers
│   │       ├── ps3rt_be.h
│   │       └── ps3rt_be_masm.asm        # Optional MASM implementation
│   ├── processor/
│   │   ├── PPU/                         # PowerPC Processor Unit
│   │   │   ├── translate/
│   │   │   │   ├── ppu_import_bridge.c  # Bridge for translated import calls
│   │   │   │   └── ppu_import_bridge.h
│   │   │   ├── ppu_disasm.cpp           # PPU disassembler
│   │   │   ├── ppu_disasm.h
│   │   │   ├── ppu_interpreter.cpp      # PPU interpreter
│   │   │   ├── ppu_interpreter.h
│   │   │   ├── ppu_island_table.h       # Null-island checks/escape ranking
│   │   │   ├── ppu_jit.cpp              # PPU JIT execution path
│   │   │   ├── ppu_jit.h
│   │   │   ├── ppu_lifter.cpp           # PPU code lifting
│   │   │   ├── ppu_lifter.h
│   │   │   ├── ppu_runtime.h            # Generated-code runtime interface
│   │   │   ├── ppu_runtime_core.h       # Shared runtime core
│   │   │   ├── ppu_runtime_core_a.h     # Runtime core section A
│   │   │   ├── ppu_runtime_core_b.h     # Runtime core section B
│   │   │   └── ppu_runtime_run.h        # Runtime execution helpers
│   │   └── SPU/                         # Synergistic Processing Unit
│   │       ├── spu_interpreter.cpp      # SPU interpreter
│   │       ├── spu_interpreter.h
│   │       ├── spu_jit.cpp              # SPU JIT execution path
│   │       ├── spu_jit.h
│   │       ├── spu_local_store.cpp      # SPU local-store model
│   │       └── spu_local_store.h
│   └── ui_qt/                           # Qt 6 desktop application
│       ├── AppVersion.h                 # GUI version metadata
│       ├── CMakeLists.txt               # Qt GUI target/build integration
│       ├── EmbeddedLegal.cpp             # Embedded legal text
│       ├── EmbeddedLegal.h
│       ├── EmbeddedTerms.cpp             # Embedded terms text
│       ├── EmbeddedTerms.h
│       ├── JobWorker.cpp                 # Background job worker
│       ├── JobWorker.h
│       ├── MainWindow.cpp                # Main Qt window and UI composition
│       ├── MainWindow.h
│       ├── NativeBridge.cpp              # Bridge to native runtime APIs
│       ├── NativeBridge.h
│       ├── Settings.cpp                  # Persistent GUI settings
│       ├── Settings.h
│       ├── TermsDialog.cpp               # Terms dialog
│       ├── TermsDialog.h
│       ├── UiLogoAnim.cpp                # Logo animation
│       ├── UiLogoAnim.h
│       ├── main.cpp                      # Qt application entry point
│       └── resources.qrc                 # Qt resource collection
├── tests/
│   ├── conformance/
│   │   ├── README.md                    # Conformance test notes
│   │   └── ppu_smoke_plan.md            # PPU smoke-test plan
│   └── games/
│       └── README.md                    # Game test/validation notes
├── .gitattributes                       # Git text/binary handling
├── .gitignore                           # Excluded build/local files
├── AIFIAR.md                            # AI-assisted engineering workflow
├── CITATION.cff                         # Citation metadata
├── CMakeLists.txt                       # Main C/C++ and Qt build definition
├── CMakePresets.json                    # Configure/build presets
├── CMakeSettings.json                   # Visual Studio CMake settings
├── CODE_OF_CONDUCT.md                   # Community conduct
├── CONTRIBUTING.md                      # Contribution workflow
├── ENGINEERING.md                       # Engineering rules and validation expectations
├── LICENSE                              # Apache License 2.0
├── README.md                            # Project overview and entry links
├── ROADMAP.md                           # Development roadmap
├── SECURITY.md                          # Security reporting policy
├── STRUCTURE.md                         # This repository map
├── USER_MANUAL.md                       # User-facing operating manual
└── requirements.md                      # Build/runtime requirements
```

## Major subsystems

### Build and outputs
- The root `CMakeLists.txt` defines the native C/C++ targets and the Qt 6 Widgets desktop application.
- C and C++ language standards are configured at the root. The project uses CMake presets/settings for supported IDE workflows.
- Build outputs are staged under the configured `dist/` directory. The Windows runtime target is consolidated as `ps3rt.dll`; the build also stages runtime headers under `dist/runtime/` and copies the Vulkan runtime when available.
- `cmake/CopyVulkanRuntime.cmake` handles Vulkan runtime staging. Exact output contents depend on platform, configuration, and successfully built targets.
- `src/ui_qt/CMakeLists.txt` contains Qt GUI target configuration. The former C# WinForms UI layout is no longer part of the tracked source tree.

### Core and lifting
- `src/core/` contains ELF loading, guest memory, virtual filesystem and metadata/archive parsing, project/session state, core APIs, import mapping, and kernel/HLE interfaces.
- `src/processor/PPU/` contains PPU disassembly, interpretation, lifting, JIT-related execution, runtime interfaces, and translated import bridging.
- `src/processor/SPU/` contains SPU interpretation/JIT paths and local-store support.
- `src/native/x64/` contains host-specific helpers, including big-endian access routines and an optional MASM implementation.

### Runtime services and media
- `src/core/hle/` and adjacent HLE headers provide kernel-facing services, GCM bridges, syscall definitions, startup stages, and status/registration interfaces.
- `src/games-stuff/Audio/` contains audio decoding, host drivers, device lifecycle, mixing, DSP, voice, and cellAudio-facing code.
- `src/games-stuff/Video/` contains decoder entry points and helpers for timing, color space, demultiplexing, PAMF, and frame presentation.

### RSX graphics
- `src/RSX/` is organized into device/state and resource handling, FIFO/method processing, dispatch, draw/surface helpers, render/presentation policy, resolution, shader binding/decoding/compilation, and host backends.
- `src/RSX/Backends/Vulkan/` contains the Vulkan backend implementation, memory, command submission, and pipeline setup.
- `src/RSX/Backends/OpenGL/` contains OpenGL backend and shader/state/VAO helpers.
- `rsx_shader_bind.c/.h` tracks shader binding state, offsets/sizes/constants, and host readiness as the shader path is integrated. File presence does not establish complete title rendering.

### Desktop UI
- `src/ui_qt/` is the Qt 6 desktop application, with the main window, settings, native bridge, background job worker, terms/legal dialogs, logo animation, and Qt resources.
- `assets/ppsx33-logo.ico` is used for Windows icon/resource integration; the PNG assets are used for visual branding and documentation.

### Documentation, game records, and tests
- Root Markdown files cover project usage, contribution, engineering, AI-assisted workflow, releases, requirements, security, and repository layout.
- `docs/` contains architecture/developer references, diagrams, input mapping, PPU coverage, legal/third-party notes, and per-game analysis records. Game reports are research and diagnostic artifacts, not compatibility guarantees.
- `tests/conformance/` contains conformance notes and a PPU smoke-test plan. `tests/games/` contains game-validation guidance.
- `.github/workflows/validate.yml` and issue templates support repository validation and structured issue reporting.

## Important accuracy notes

1. **PPSX33's target is native Windows PlayStation 3 recompilation.** A source file, subsystem, lift report, or successful build does not prove that a commercial title boots or is playable.
2. **The current GUI is Qt 6.** Do not document the removed C# WinForms files as current source files.
3. **The runtime layout is consolidated.** Refer to the root build configuration for the current `ps3rt.dll` target and staging rules instead of assuming the older split render/resolution/API DLL layout.
4. **Generated/build artifacts are not source-tree entries.** Paths such as `build/`, `dist/`, generated game projects, executables, PDB/ILK files, and staged DLLs may appear locally but are not listed as tracked source files here.
5. **Game-specific notes are evidence, not guarantees.** Consult the corresponding analysis/lift reports and the current architecture/roadmap before making claims about execution or graphics coverage.
