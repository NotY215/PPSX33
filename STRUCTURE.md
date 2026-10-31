# 📦 PPSX33 Repository Structure

**🌐 Website:** https://ppsx33.xo.je/  
**🗓️ Last updated:** 2026-10-31

This document maps the current PPSX33 source layout and the files that participate in building or running the application. It keeps the emoji-based tree and inline comments so each component's responsibility is visible at a glance.

> 🧭 **Scope:** This is a build/source map, not a complete repository inventory. Documentation folders and standalone Markdown files are omitted from the tree to keep the focus on the application. Nothing has been deleted from the repository.

```text
📦 PPSX33/
├── ⚙️ CMakeLists.txt                    # Defines native targets, GUI publish, and output staging
├── ⚙️ CMakePresets.json                 # Configure/build presets, including x64 Release
├── ⚙️ CMakeSettings.json                # Visual Studio CMake configuration
├── ⚙️ Directory.Build.props             # Shared MSBuild properties
├── 🧩 PS3Recomp.sln                     # Visual Studio solution entry point
├── 🪟 launch.vs.json                    # Visual Studio launch configuration
├── 🎨 assets/
│   └── ppsx33-logo.ico                  # Application icon embedded by the GUI project
├── 🛠️ cmake/
│   └── CopyVulkanRuntime.cmake          # Copies the Vulkan runtime DLL into build output
├── 📦 redist/                           # Runtime redistributables staged with the build
└── 📂 src/                              # Main application source tree
    ├── 📂 cli/                          # Command-line entry points and headless helpers
    │   └── ps3_cli.cpp                  # CLI entry for command-line operations
    ├── 📂 core/                         # PS3 system services, memory, filesystems, and HLE
    │   ├── 📂 fs/                       # Virtual filesystem and metadata/archive parsing
    │   │   ├── psarc_parser.cpp          # Reads supported PSARC archive data
    │   │   ├── psarc_parser.h            # PSARC parser interface
    │   │   ├── sfo_parser.cpp            # Reads PARAM.SFO metadata
    │   │   ├── sfo_parser.h              # PARAM.SFO parser interface
    │   │   ├── vfs.cpp                   # Maps guest filesystem paths to host storage
    │   │   └── vfs.h                     # Virtual filesystem interface
    │   ├── 📂 import/                    # Guest import/NID binding support
    │   │   ├── import_stub_map.c         # Static import-stub mapping implementation
    │   │   └── import_stub_map.h         # Import-stub mapping declarations
    │   ├── 📂 loaders/                   # Executable loading
    │   │   ├── elf_loader.cpp            # Parses supported ELF headers and segments
    │   │   └── elf_loader.h              # ELF loader interface
    │   ├── assets.cpp                    # Asset-related core helpers
    │   ├── hle_hw_progress.cpp           # Optional/reference hardware-progress implementation
    │   ├── hle_kernel.cpp                # Kernel HLE services and syscall handling
    │   ├── hle_kernel.h                  # Public kernel HLE interface
    │   ├── hle_kernel_internal.h         # Internal kernel declarations
    │   ├── hle_kernel_syscalls.inl        # Kernel syscall dispatch bodies
    │   ├── hle_log.cpp                   # Optional/reference logging implementation
    │   ├── hle_registry.h                 # HLE registration declarations
    │   ├── hle_stage.h                    # HLE startup-stage definitions
    │   ├── hle_status.h                   # HLE status definitions
    │   ├── lv2_syscalls.inl               # LV2 syscall definitions
    │   ├── memory.cpp                     # Guest memory management
    │   ├── memory.h                       # Guest memory interface
    │   ├── nid_table.h                    # NID lookup data/interface
    │   ├── project.cpp                    # Project and game-session logic
    │   ├── project.h                      # Project/session declarations
    │   ├── ps3_util.c                     # Shared low-level PS3 utilities
    │   ├── ps3_util.h                     # Utility declarations
    │   ├── ps3core.h                      # Core API and shared definitions
    │   └── ps3core_api.cpp                # Core API implementation
    ├── 📂 games-stuff/                   # Game-facing subsystems and media support
    │   ├── 📂 Audio/                     # Audio HLE, mixing, drivers, and decoders
    │   │   ├── 📂 Decoders/              # Audio format decoders
    │   │   │   ├── decode_adpcm.cpp       # ADPCM/VAG decoding
    │   │   │   ├── decode_adpcm.h         # ADPCM decoder interface
    │   │   │   ├── decode_at3.cpp         # ATRAC3-family decoding
    │   │   │   ├── decode_at3.h           # ATRAC decoder interface
    │   │   │   ├── decode_celp.cpp        # CELP voice/audio decoding
    │   │   │   ├── decode_celp.h          # CELP decoder interface
    │   │   ├── 📂 Drivers/                # Host audio output backends
    │   │   │   ├── audio_cubeb_backend.cpp # cubeb backend implementation
    │   │   │   ├── audio_sdl2_backend.cpp  # SDL2 audio backend implementation
    │   │   │   └── audio_xaudio2_backend.cpp # Windows XAudio2 backend
    │   │   ├── audio_aub.cpp              # Audio buffer/memory helpers
    │   │   ├── audio_aub.h                # Audio buffer declarations
    │   │   ├── audio_device.cpp            # Host audio device lifecycle
    │   │   ├── audio_device.h              # Audio device interface
    │   │   ├── audio_dsp.cpp               # Audio DSP and effects
    │   │   ├── audio_dsp.h                 # DSP declarations
    │   │   ├── audio_mixer.cpp             # Mixes audio streams
    │   │   ├── audio_mixer.h               # Mixer interface
    │   │   ├── audio_voice.cpp              # Voice playback state and controls
    │   │   ├── audio_voice.h                # Voice declarations
    │   │   ├── libaudio.cpp                 # cellAudio HLE implementation
    │   │   └── libaudio.h                   # cellAudio HLE declarations
    │   └── 📂 Video/                      # Video playback and decoding helpers
    │       ├── 📂 Decoders/               # Video format decoders
    │       │   ├── decode_divx.cpp         # DivX decoding path
    │       │   ├── decode_divx.h           # DivX decoder interface
    │       │   ├── decode_h264.cpp         # H.264/AVC decoding path
    │       │   ├── decode_h264.h           # H.264 decoder interface
    │       │   ├── decode_mpeg2.cpp        # MPEG-2 decoding path
    │       │   └── decode_mpeg2.h          # MPEG-2 decoder interface
    │       ├── video_clock.cpp             # Video timing and frame pacing
    │       ├── video_clock.h               # Video clock interface
    │       ├── video_color_space.cpp       # YUV/RGB color conversion helpers
    │       ├── video_color_space.h         # Color conversion declarations
    │       ├── video_demuxer.cpp            # Video packet demultiplexing
    │       ├── video_demuxer.h              # Demuxer interface
    │       ├── video_pamf.cpp               # Sony PAMF container handling
    │       ├── video_pamf.h                 # PAMF parser declarations
    │       ├── video_presenter.cpp          # Presents decoded video frames
    │       └── video_presenter.h            # Video presenter interface
    ├── 📂 native/                         # Host-native runtime helpers
    │   └── 📂 x64/                        # x64-specific implementations
    │       ├── ps3rt_be.c                  # Big-endian load/store helpers
    │       ├── ps3rt_be.h                  # Big-endian helper declarations
    │       └── ps3rt_be_masm.asm           # Optional MASM byte-swap helper
    ├── 📂 processor/                      # PS3 CPU execution and translation
    │   ├── 📂 PPU/                        # PowerPC Processor Unit
    │   │   ├── 📂 translate/              # Import bridge for translated code
    │   │   │   ├── ppu_import_bridge.c     # Resolves translated import calls
    │   │   │   └── ppu_import_bridge.h     # Import bridge interface
    │   │   ├── ppu_disasm.cpp              # PPU instruction disassembler
    │   │   ├── ppu_disasm.h                # Disassembler interface
    │   │   ├── ppu_interpreter.cpp         # PPU instruction interpreter
    │   │   ├── ppu_interpreter.h           # Interpreter interface
    │   │   ├── ppu_jit.cpp                 # PPU JIT implementation
    │   │   ├── ppu_jit.h                   # PPU JIT interface
    │   │   ├── ppu_lifter.cpp              # Lifts PPU blocks into host-side code
    │   │   ├── ppu_lifter.h                # PPU lifting interface
    │   │   ├── ppu_runtime.h               # PPU runtime declarations
    │   │   ├── ppu_runtime_core.h          # Shared runtime core
    │   │   ├── ppu_runtime_core_a.h        # Runtime core section A
    │   │   ├── ppu_runtime_core_b.h        # Runtime core section B
    │   │   └── ppu_runtime_run.h           # Runtime execution helpers
    │   └── 📂 SPU/                         # Synergistic Processing Unit
    │       ├── spu_interpreter.cpp          # SPU instruction interpreter
    │       ├── spu_interpreter.h            # Interpreter interface
    │       ├── spu_jit.cpp                  # SPU JIT implementation
    │       ├── spu_jit.h                    # SPU JIT interface
    │       ├── spu_local_store.cpp          # SPU local-store memory model
    │       └── spu_local_store.h            # Local-store interface
    ├── 📂 RSX/                            # PS3 graphics processor emulation
    │   ├── 📂 Backends/                   # Host graphics APIs
    │   │   ├── 📂 OpenGL/                 # OpenGL graphics backend
    │   │   │   ├── rsx_gl_backend.cpp      # OpenGL backend setup and submission
    │   │   │   ├── rsx_gl_backend.h        # OpenGL backend interface
    │   │   │   ├── rsx_gl_shaders.cpp      # GLSL shader compilation helpers
    │   │   │   ├── rsx_gl_state.cpp        # OpenGL pipeline state
    │   │   │   └── rsx_gl_vao.cpp          # Vertex array/buffer bindings
    │   │   └── 📂 Vulkan/                 # Vulkan graphics backend
    │   │       ├── rsx_vk_backend.cpp      # Vulkan device/backend setup
    │   │       ├── rsx_vk_backend.h        # Vulkan backend interface
    │   │       ├── rsx_vk_command.cpp      # Command-buffer submission helpers
    │   │       ├── rsx_vk_memory.cpp       # GPU memory allocation helpers
    │   │       └── rsx_vk_pipelines.cpp    # Vulkan pipeline state
    │   ├── 📂 fifo/                       # RSX FIFO command handling
    │   │   ├── rsx_method_table.c          # Classifies RSX methods
    │   │   └── rsx_method_table.h          # Method-table declarations
    │   ├── 📂 present/                    # Frame presentation queue
    │   │   ├── rsx_present_queue.c         # Queues asynchronous/double-buffered presents
    │   │   └── rsx_present_queue.h         # Present queue declarations
    │   ├── rsx_backend_interface.h         # Common backend contract
    │   ├── rsx_device.cpp                  # RSX device/register handling
    │   ├── rsx_device.h                    # Device interface
    │   ├── rsx_fifo.cpp                    # FIFO command processing
    │   ├── rsx_fifo.h                      # FIFO interface
    │   ├── rsx_fifo_guest_put_notes.h      # Notes on guest PUT/FIFO behavior
    │   ├── rsx_framebuffer.cpp             # Framebuffer and render-target handling
    │   ├── rsx_framebuffer.h               # Framebuffer interface
    │   ├── rsx_memory.cpp                  # Guest RSX memory management
    │   ├── rsx_memory.h                    # RSX memory interface
    │   ├── rsx_shader_compiler.cpp         # Shader translation/compilation
    │   ├── rsx_shader_compiler.h           # Shader compiler interface
    │   ├── rsx_shader_decoder.cpp          # RSX microcode decoding
    │   ├── rsx_shader_decoder.h            # Shader decoder interface
    │   ├── rsx_state.cpp                   # Graphics pipeline state tracking
    │   ├── rsx_state.h                     # State declarations
    │   ├── rsx_texture.cpp                 # Texture format/layout handling
    │   └── rsx_texture.h                   # Texture interface
    └── 📂 UI/                             # Windows Forms UI and native panels
        ├── 📂 panels/                     # Native debugger, game list, and settings panels
        │   ├── debugger_panel.cpp          # Debugger panel implementation
        │   ├── debugger_panel.h            # Debugger panel interface
        │   ├── game_list_panel.cpp         # Game list panel implementation
        │   ├── game_list_panel.h           # Game list panel interface
        │   ├── settings_panel.cpp          # Settings panel implementation
        │   └── settings_panel.h             # Settings panel interface
        ├── 📂 Tabs/                       # Additional GUI tabs
        │   └── LegalTab.cs                 # Legal notice tab
        ├── 📂 Ui/                         # Reusable UI styling and effects
        │   ├── LiquidBackdrop.cs           # Liquid/glass backdrop effects
        │   └── UiKit.cs                    # Shared UI components and styling
        ├── EmbeddedLegal.cs                # Embedded legal text handling
        ├── EmbeddedTerms.cs                # Embedded terms text handling
        ├── KeyMap.cs                       # Controller and keyboard mappings
        ├── MainForm.Build.cs               # Build workflow and progress handling
        ├── MainForm.Controller.cs          # Controller/input handling
        ├── MainForm.Decrypt.cs             # Decryption workflow
        ├── MainForm.Workflow.cs            # Main application workflow
        ├── MainForm.cs                     # Main WinForms window
        ├── MarkdownHtml.cs                 # Markdown-to-HTML rendering helpers
        ├── Native.cs                       # Native DLL/API interop
        ├── PS3Recomp.Gui.csproj             # .NET GUI project and publish settings
        ├── Program.cs                      # GUI application entry point
        ├── PromptDialog.cs                 # Prompt/dialog UI
        ├── Ps3Visuals.cs                   # PS3-inspired visual components
        ├── Rpcs3Launcher.cs                # RPCS3 launch integration
        ├── Settings.cs                     # Persistent application settings
        └── TermsForm.cs                    # Terms window
```

## 🏗️ Build Entry Points

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | 🧱 Defines native libraries, GUI publishing, runtime staging, and build output layout. |
| `CMakePresets.json` | 🎛️ Provides repeatable CMake configure/build presets. |
| `CMakeSettings.json` | 🪟 Supplies Visual Studio's CMake configuration. |
| `PS3Recomp.sln` | 🧩 Opens the solution in Visual Studio. |
| `src/UI/PS3Recomp.Gui.csproj` | 🖥️ Builds the Windows Forms GUI targeting .NET 8. |
| `assets/ppsx33-logo.ico` | 🎨 Application icon referenced by the GUI project. |
| `cmake/CopyVulkanRuntime.cmake` | 🎮 Stages the Vulkan runtime DLL when required. |

## 📝 Implementation Notes

- 🧠 **Core / HLE:** `src/core/` owns guest memory, filesystem helpers, executable loading, kernel services, and import mapping.
- 🧮 **CPU:** `src/processor/PPU/` contains interpretation, disassembly, lifting, and runtime support; `src/processor/SPU/` contains SPU execution and local-store code.
- 🎮 **Graphics:** `src/RSX/` contains the graphics abstraction, FIFO handling, memory/state, shaders, and OpenGL/Vulkan backend code.
- 🔊 **Audio and video:** `src/games-stuff/` groups host audio drivers, decoders, mixing, video timing, demuxing, and presentation.
- 🪟 **Frontend:** `src/UI/` contains the WinForms app and its supporting UI components; native panels remain under `src/UI/panels/`.
- 🧰 **Native runtime:** `src/native/x64/` contains host-specific endian helpers, while `src/processor/PPU/translate/` bridges translated code to import resolution.
- 📚 **Omitted from this tree:** `docs/`, `tests/`, `releases/`, standalone `.md` files, issue templates, and other repository-management content. They remain in the repository and are not part of this focused source/build listing.
