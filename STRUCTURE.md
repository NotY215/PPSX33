# 📦 PPSX33 Build and Source Structure

**🌐 Website:** https://ppsx33.xo.je/

**🗓️ Last updated:** 2026-11-03

This map documents the current CMake configuration and the native source layout, including the Qt desktop application and per-game Modify launcher.

> 🧭 **Scope:** Only the root build configuration, helper CMake script, and files under `src/` are shown. Documentation, tests, assets, redistributables, and generated build output are intentionally omitted. The root `CMakeLists.txt` is the authoritative build definition; `src/ui_qt/CMakeLists.txt` is retained as a separate/legacy configuration file.

```text
📦 PPSX33/
├── ⚙️ CMakeLists.txt                    # Main native and Qt application build definition
├── ⚙️ CMakePresets.json                 # CMake configure/build presets
├── ⚙️ CMakeSettings.json                # Visual Studio CMake configuration
├── 🛠️ cmake/
│   └── CopyVulkanRuntime.cmake          # Stages Vulkan runtime beside build output
└── 📂 src/
    ├── 📂 cli/
    │   └── ps3_cli.cpp                  # Command-line entry point
    ├── 📂 core/                         # Guest system, memory, filesystem, and HLE
    │   ├── 📂 fs/
    │   │   ├── psarc_parser.cpp          # PSARC archive parsing
    │   │   ├── psarc_parser.h
    │   │   ├── sfo_parser.cpp            # PARAM.SFO metadata parsing
    │   │   ├── sfo_parser.h
    │   │   ├── vfs.cpp                   # Guest-to-host virtual filesystem
    │   │   └── vfs.h
    │   ├── 📂 hle/
    │   │   ├── 📂 gcm/
    │   │   │   ├── gcm_control_bridge.c  # GCM control bridge
    │   │   │   ├── gcm_flip_label.c      # GCM flip-label handling
    │   │   │   └── gcm_flip_label.h
    │   │   ├── spurs_stub.h
    │   │   ├── sysmodule_progress.c      # Sysmodule progress/status helpers
    │   │   └── sysmodule_progress.h
    │   ├── 📂 import/
    │   │   ├── import_stub_map.c         # Guest import/NID stub mapping
    │   │   └── import_stub_map.h
    │   ├── 📂 loaders/
    │   │   ├── elf_loader.cpp            # ELF image loading
    │   │   └── elf_loader.h
    │   ├── 📂 stats/
    │   │   ├── host_stats.c              # Host/runtime statistics
    │   │   └── host_stats.h
    │   ├── assets.cpp                    # Core asset helpers
    │   ├── hle_hw_progress.cpp           # Hardware progress helpers
    │   ├── hle_kernel.cpp                # Kernel HLE and syscall implementation
    │   ├── hle_kernel.h
    │   ├── hle_kernel_internal.h
    │   ├── hle_kernel_syscalls.inl       # Kernel syscall implementation include
    │   ├── hle_log.cpp                   # HLE logging helpers
    │   ├── hle_registry.h
    │   ├── hle_stage.h
    │   ├── hle_status.h
    │   ├── lv2_syscalls.inl              # LV2 syscall definitions
    │   ├── memory.cpp                    # Guest memory management
    │   ├── memory.h
    │   ├── nid_table.h                   # NID lookup definitions
    │   ├── project.cpp                   # Project/game-session logic
    │   ├── project.h
    │   ├── ps3_util.c                    # Shared low-level PS3 utilities
    │   ├── ps3_util.h
    │   ├── ps3core_api.cpp               # Core API implementation
    │   └── ps3core.h
    ├── 📂 games-stuff/                   # Audio and video services
    │   ├── 📂 Audio/
    │   │   ├── 📂 Decoders/
    │   │   │   ├── decode_adpcm.cpp      # ADPCM/VAG decoding
    │   │   │   ├── decode_adpcm.h
    │   │   │   ├── decode_at3.cpp        # ATRAC-family decoding
    │   │   │   ├── decode_at3.h
    │   │   │   ├── decode_celp.cpp       # CELP decoding
    │   │   │   └── decode_celp.h
    │   │   ├── 📂 Drivers/
    │   │   │   ├── audio_cubeb_backend.cpp # cubeb output backend
    │   │   │   ├── audio_sdl2_backend.cpp  # SDL2 output backend
    │   │   │   └── audio_xaudio2_backend.cpp # Windows XAudio2 output backend
    │   │   ├── audio_aub.cpp             # Audio buffer helpers
    │   │   ├── audio_aub.h
    │   │   ├── audio_device.cpp          # Host audio device lifecycle
    │   │   ├── audio_device.h
    │   │   ├── audio_dsp.cpp             # DSP and effects
    │   │   ├── audio_dsp.h
    │   │   ├── audio_mixer.cpp           # Audio stream mixing
    │   │   ├── audio_mixer.h
    │   │   ├── audio_voice.cpp            # Voice playback state
    │   │   ├── audio_voice.h
    │   │   ├── libaudio.cpp               # cellAudio HLE implementation
    │   │   └── libaudio.h
    │   └── 📂 Video/
    │       ├── 📂 Decoders/
    │       │   ├── decode_divx.cpp        # DivX decoding
    │       │   ├── decode_divx.h
    │       │   ├── decode_h264.cpp        # H.264/AVC decoding
    │       │   ├── decode_h264.h
    │       │   ├── decode_mpeg2.cpp       # MPEG-2 decoding
    │       │   └── decode_mpeg2.h
    │       ├── video_clock.cpp            # Video timing/frame pacing
    │       ├── video_clock.h
    │       ├── video_color_space.cpp      # Color-space conversion
    │       ├── video_color_space.h
    │       ├── video_demuxer.cpp          # Video packet demultiplexing
    │       ├── video_demuxer.h
    │       ├── video_pamf.cpp             # PAMF container handling
    │       ├── video_pamf.h
    │       ├── video_presenter.cpp        # Decoded-frame presentation
    │       └── video_presenter.h
    ├── 📂 native/
    │   └── 📂 x64/                       # x64 host-specific runtime helpers
    │       ├── ps3rt_be.c                 # Big-endian load/store helpers
    │       ├── ps3rt_be.h
    │       └── ps3rt_be_masm.asm         # Optional MASM implementation
    ├── 📂 processor/
    │   ├── 📂 PPU/                       # PowerPC Processor Unit
    │   │   ├── 📂 translate/
    │   │   │   ├── ppu_import_bridge.c   # Bridge for translated imports
    │   │   │   └── ppu_import_bridge.h
    │   │   ├── ppu_disasm.cpp            # PPU instruction disassembler
    │   │   ├── ppu_disasm.h
    │   │   ├── ppu_interpreter.cpp       # PPU instruction interpreter
    │   │   ├── ppu_interpreter.h
    │   │   ├── ppu_island_table.h        # PPU null-island checks
    │   │   ├── ppu_jit.cpp               # PPU JIT execution path
    │   │   ├── ppu_jit.h
    │   │   ├── ppu_lifter.cpp            # PPU code lifting
    │   │   ├── ppu_lifter.h
    │   │   ├── ppu_runtime.h             # Generated-code runtime interface
    │   │   ├── ppu_runtime_core.h
    │   │   ├── ppu_runtime_core_a.h
    │   │   ├── ppu_runtime_core_b.h
    │   │   └── ppu_runtime_run.h
    │   └── 📂 SPU/                       # Synergistic Processing Unit
    │       ├── spu_interpreter.cpp       # SPU instruction interpreter
    │       ├── spu_interpreter.h
    │       ├── spu_jit.cpp               # SPU JIT execution path
    │       ├── spu_jit.h
    │       ├── spu_local_store.cpp       # SPU local-store model
    │       └── spu_local_store.h
    ├── 📂 RSX/                           # RSX graphics processor emulation
    │   ├── 📂 Backends/
    │   │   ├── 📂 OpenGL/
    │   │   │   ├── rsx_gl_backend.cpp    # OpenGL backend setup/submission
    │   │   │   ├── rsx_gl_backend.h
    │   │   │   ├── rsx_gl_shaders.cpp    # GLSL shader helpers
    │   │   │   ├── rsx_gl_state.cpp      # OpenGL state handling
    │   │   │   └── rsx_gl_vao.cpp        # Vertex-array/buffer binding
    │   │   └── 📂 Vulkan/
    │   │       ├── rsx_vk_backend.cpp    # Vulkan backend/device setup
    │   │       ├── rsx_vk_backend.h
    │   │       ├── rsx_vk_command.cpp    # Command-buffer submission
    │   │       ├── rsx_vk_memory.cpp     # Vulkan memory helpers
    │   │       └── rsx_vk_pipelines.cpp  # Graphics pipeline setup
    │   ├── 📂 dispatch/
    │   │   ├── rsx_dispatch_sink.c       # Decoded-method dispatch sink
    │   │   └── rsx_dispatch_sink.h
    │   ├── 📂 draw/
    │   │   ├── rsx_draw_batch.c          # Draw batching
    │   │   ├── rsx_draw_batch.h
    │   │   ├── rsx_surface_fmt.c         # Surface format helpers
    │   │   └── rsx_surface_fmt.h
    │   ├── 📂 fifo/
    │   │   ├── rsx_fifo_drain.c          # FIFO command draining
    │   │   ├── rsx_fifo_drain.h
    │   │   ├── rsx_method_table.c        # RSX method dispatch table
    │   │   └── rsx_method_table.h
    │   ├── 📂 methods/
    │   │   └── rsx_nv4097_surface.c      # NV4097 surface-method handling
    │   ├── 📂 present/
    │   │   ├── rsx_present_policy.c      # Presentation policy
    │   │   ├── rsx_present_policy.h
    │   │   ├── rsx_present_queue.c       # Frame presentation queue
    │   │   └── rsx_present_queue.h
    │   ├── 📂 render/
    │   │   ├── rsx_light_present.c       # Lightweight presentation path
    │   │   ├── rsx_light_present.h
    │   │   ├── rsx_render_frame.c        # Frame orchestration
    │   │   └── rsx_render_frame.h
    │   ├── 📂 resolution/
    │   │   ├── rsx_resolution.c          # Resolution helpers
    │   │   └── rsx_resolution.h
    │   ├── rsx_backend_interface.h       # Shared backend contract
    │   ├── rsx_device.cpp                # RSX device/register handling
    │   ├── rsx_device.h
    │   ├── rsx_exports.c                 # Runtime exports
    │   ├── rsx_fifo.cpp                  # FIFO command processing
    │   ├── rsx_fifo.h
    │   ├── rsx_fifo_guest_put_notes.h    # Guest PUT/FIFO notes
    │   ├── rsx_framebuffer.cpp           # Framebuffer/render-target handling
    │   ├── rsx_framebuffer.h
    │   ├── rsx_memory.cpp                # Guest RSX memory management
    │   ├── rsx_memory.h
    │   ├── rsx_method_table.c
    │   ├── rsx_shader_bind.c             # Shader binding state
    │   ├── rsx_shader_bind.h
    │   ├── rsx_shader_compiler.cpp       # Shader translation/compilation
    │   ├── rsx_shader_compiler.h
    │   ├── rsx_shader_decoder.cpp         # RSX microcode decoding
    │   ├── rsx_shader_decoder.h
    │   ├── rsx_state.cpp                 # Graphics pipeline state
    │   ├── rsx_state.h
    │   ├── rsx_texture.cpp               # Texture format/layout handling
    │   └── rsx_texture.h
    └── 📂 ui_qt/                         # Qt 6 desktop application
        ├── AppVersion.h                  # GUI version metadata
        ├── CMakeLists.txt                # Standalone CMake configuration; root build defines the Qt target
        ├── EmbeddedLegal.cpp             # Embedded legal text
        ├── EmbeddedLegal.h
        ├── EmbeddedTerms.cpp              # Embedded terms text
        ├── EmbeddedTerms.h
        ├── JobWorker.cpp                  # Background job worker
        ├── JobWorker.h
        ├── MainWindow.cpp                # Main window and UI composition
        ├── MainWindow.h
        ├── NativeBridge.cpp              # Bridge to native runtime APIs
        ├── NativeBridge.h
        ├── main.cpp                       # Qt application entry point
        ├── resources.qrc                 # Qt resource collection
        ├── Settings.cpp                   # Persistent GUI settings
        ├── Settings.h
        ├── TermsDialog.cpp                # Terms dialog
        ├── TermsDialog.h
        ├── UiLogoAnim.cpp                 # Logo animation
        └── UiLogoAnim.h
```
