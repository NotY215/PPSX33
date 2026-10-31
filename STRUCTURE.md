# PPSX33 Build Structure

**Repository:** [NotY215/PPSX33](https://github.com/NotY215/PPSX33)  
**Last updated:** 2026-10-31

This file documents the files and directories that make up the PPSX33 application and its build pipeline. Documentation-only folders and Markdown files are intentionally omitted. This is a structure reference only; it does not delete or move anything in the repository.

```text
PPSX33/
├── CMakeLists.txt                    # Native targets, GUI publish, and output staging
├── CMakePresets.json                 # Configure/build presets
├── CMakeSettings.json                # Visual Studio CMake settings
├── Directory.Build.props             # Shared MSBuild properties
├── PS3Recomp.sln                     # Visual Studio solution
├── launch.vs.json                    # Visual Studio launch configuration
├── assets/
│   └── ppsx33-logo.ico               # GUI application icon
├── cmake/
│   └── CopyVulkanRuntime.cmake       # Stages the Vulkan runtime DLL
├── redist/                           # Files staged into the build output
└── src/
    ├── cli/
    │   └── ps3_cli.cpp               # Command-line entry point
    ├── core/
    │   ├── fs/
    │   │   ├── psarc_parser.cpp
    │   │   ├── psarc_parser.h
    │   │   ├── sfo_parser.cpp
    │   │   ├── sfo_parser.h
    │   │   ├── vfs.cpp
    │   │   └── vfs.h
    │   ├── import/
    │   │   ├── import_stub_map.c
    │   │   └── import_stub_map.h
    │   ├── loaders/
    │   │   ├── elf_loader.cpp
    │   │   └── elf_loader.h
    │   ├── assets.cpp
    │   ├── hle_hw_progress.cpp
    │   ├── hle_kernel.cpp
    │   ├── hle_kernel.h
    │   ├── hle_kernel_internal.h
    │   ├── hle_kernel_syscalls.inl
    │   ├── hle_log.cpp
    │   ├── hle_registry.h
    │   ├── hle_stage.h
    │   ├── hle_status.h
    │   ├── lv2_syscalls.inl
    │   ├── memory.cpp
    │   ├── memory.h
    │   ├── nid_table.h
    │   ├── project.cpp
    │   ├── project.h
    │   ├── ps3_util.c
    │   ├── ps3_util.h
    │   ├── ps3core.h
    │   └── ps3core_api.cpp
    ├── games-stuff/
    │   ├── Audio/
    │   │   ├── Decoders/
    │   │   │   ├── decode_adpcm.cpp
    │   │   │   ├── decode_adpcm.h
    │   │   │   ├── decode_at3.cpp
    │   │   │   ├── decode_at3.h
    │   │   │   ├── decode_celp.cpp
    │   │   │   └── decode_celp.h
    │   │   ├── Drivers/
    │   │   │   ├── audio_cubeb_backend.cpp
    │   │   │   ├── audio_sdl2_backend.cpp
    │   │   │   └── audio_xaudio2_backend.cpp
    │   │   ├── audio_aub.cpp
    │   │   ├── audio_aub.h
    │   │   ├── audio_device.cpp
    │   │   ├── audio_device.h
    │   │   ├── audio_dsp.cpp
    │   │   ├── audio_dsp.h
    │   │   ├── audio_mixer.cpp
    │   │   ├── audio_mixer.h
    │   │   ├── audio_voice.cpp
    │   │   ├── audio_voice.h
    │   │   ├── libaudio.cpp
    │   │   └── libaudio.h
    │   └── Video/
    │       ├── Decoders/
    │       │   ├── decode_divx.cpp
    │       │   ├── decode_divx.h
    │       │   ├── decode_h264.cpp
    │       │   ├── decode_h264.h
    │       │   ├── decode_mpeg2.cpp
    │       │   └── decode_mpeg2.h
    │       ├── video_clock.cpp
    │       ├── video_clock.h
    │       ├── video_color_space.cpp
    │       ├── video_color_space.h
    │       ├── video_demuxer.cpp
    │       ├── video_demuxer.h
    │       ├── video_pamf.cpp
    │       ├── video_pamf.h
    │       ├── video_presenter.cpp
    │       └── video_presenter.h
    ├── native/
    │   └── x64/
    │       ├── ps3rt_be.c
    │       ├── ps3rt_be.h
    │       └── ps3rt_be_masm.asm
    ├── processor/
    │   ├── PPU/
    │   │   ├── translate/
    │   │   │   ├── ppu_import_bridge.c
    │   │   │   └── ppu_import_bridge.h
    │   │   ├── ppu_disasm.cpp
    │   │   ├── ppu_disasm.h
    │   │   ├── ppu_interpreter.cpp
    │   │   ├── ppu_interpreter.h
    │   │   ├── ppu_jit.cpp
    │   │   ├── ppu_jit.h
    │   │   ├── ppu_lifter.cpp
    │   │   ├── ppu_lifter.h
    │   │   ├── ppu_runtime.h
    │   │   ├── ppu_runtime_core.h
    │   │   ├── ppu_runtime_core_a.h
    │   │   ├── ppu_runtime_core_b.h
    │   │   └── ppu_runtime_run.h
    │   └── SPU/
    │       ├── spu_interpreter.cpp
    │       ├── spu_interpreter.h
    │       ├── spu_jit.cpp
    │       ├── spu_jit.h
    │       ├── spu_local_store.cpp
    │       └── spu_local_store.h
    ├── RSX/
    │   ├── Backends/
    │   │   ├── OpenGL/
    │   │   │   ├── rsx_gl_backend.cpp
    │   │   │   ├── rsx_gl_backend.h
    │   │   │   ├── rsx_gl_shaders.cpp
    │   │   │   ├── rsx_gl_state.cpp
    │   │   │   └── rsx_gl_vao.cpp
    │   │   └── Vulkan/
    │   │       ├── rsx_vk_backend.cpp
    │   │       ├── rsx_vk_backend.h
    │   │       ├── rsx_vk_command.cpp
    │   │       ├── rsx_vk_memory.cpp
    │   │       └── rsx_vk_pipelines.cpp
    │   ├── fifo/
    │   │   ├── rsx_method_table.c
    │   │   └── rsx_method_table.h
    │   ├── present/
    │   │   ├── rsx_present_queue.c
    │   │   └── rsx_present_queue.h
    │   ├── rsx_backend_interface.h
    │   ├── rsx_device.cpp
    │   ├── rsx_device.h
    │   ├── rsx_fifo.cpp
    │   ├── rsx_fifo.h
    │   ├── rsx_fifo_guest_put_notes.h
    │   ├── rsx_framebuffer.cpp
    │   ├── rsx_framebuffer.h
    │   ├── rsx_memory.cpp
    │   ├── rsx_memory.h
    │   ├── rsx_shader_compiler.cpp
    │   ├── rsx_shader_compiler.h
    │   ├── rsx_shader_decoder.cpp
    │   ├── rsx_shader_decoder.h
    │   ├── rsx_state.cpp
    │   ├── rsx_state.h
    │   ├── rsx_texture.cpp
    │   └── rsx_texture.h
    └── UI/
        ├── panels/
        │   ├── debugger_panel.cpp
        │   ├── debugger_panel.h
        │   ├── game_list_panel.cpp
        │   ├── game_list_panel.h
        │   ├── settings_panel.cpp
        │   └── settings_panel.h
        ├── Tabs/
        │   └── LegalTab.cs
        ├── Ui/
        │   ├── LiquidBackdrop.cs
        │   └── UiKit.cs
        ├── EmbeddedLegal.cs
        ├── EmbeddedTerms.cs
        ├── KeyMap.cs
        ├── MainForm.Build.cs
        ├── MainForm.Controller.cs
        ├── MainForm.Decrypt.cs
        ├── MainForm.Workflow.cs
        ├── MainForm.cs
        ├── MarkdownHtml.cs
        ├── Native.cs
        ├── PS3Recomp.Gui.csproj
        ├── Program.cs
        ├── PromptDialog.cs
        ├── Ps3Visuals.cs
        ├── Rpcs3Launcher.cs
        ├── Settings.cs
        └── TermsForm.cs
```

## Build entry points

- **CMake:** `CMakeLists.txt` defines the native `ps3core` and Windows `ps3rt` libraries, stages runtime headers and redistributables, and publishes the GUI when the .NET SDK is available.
- **CMake presets:** `CMakePresets.json` provides the `x64-release` configure and build presets.
- **GUI project:** `src/UI/PS3Recomp.Gui.csproj` builds the Windows Forms application targeting `.NET 8`.
- **Solution:** `PS3Recomp.sln` opens the project in Visual Studio.
- **Application icon:** `assets/ppsx33-logo.ico` is referenced by the GUI project.
- **Vulkan staging:** `cmake/CopyVulkanRuntime.cmake` is invoked by the native build to stage the Vulkan runtime.

## Scope

This is a build-focused map, not a complete repository inventory. It intentionally omits `docs/`, `tests/`, `releases/`, root-level Markdown documentation, issue templates, and other project-management files. Those files remain in the repository and are not deleted by this document update.
