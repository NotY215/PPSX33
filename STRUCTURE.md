# PPSX33 repository structure

**Website:** https://ppsx33.xo.je/

```
Project_Root/
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
├── LICENSE
├── PS3Recomp.sln
├── README.md
├── ROADMAP.md
├── SECURITY.md
├── STRUCTURE.md
├── USER_MANUAL.md
├── launch.vs.json
├── requirements.md
├── .github/
│   ├── ISSUE_TEMPLATE/
│   └── workflows/
├── assets/
│   ├── PPSX33 PS3 Controller Key Mapping.png
│   ├── ppsx33-banner.png
│   ├── ppsx33-logo.ico
│   └── ppsx33-logo.png
├── docs/
│   ├── ARCHITECTURE.md
│   ├── DEVELOPER_GUIDE.md
│   ├── DIAGRAMS.md
│   ├── KEY_MAPPING.md
│   ├── PPU_COVERAGE.md
│   ├── legal/                 # Legal notices (LICENSE stays at root)
│   │   ├── LEGAL.md
│   │   ├── TERMS.md
│   │   ├── PRIVACY.md
│   │   ├── NOTICE
│   │   ├── THIRD_PARTY.md
│   │   └── GOVERNANCE.md
│   └── games/                 # Per-title notes and logs
├── redist/
│   └── README.txt
├── releases/
│   ├── PPSX33-Alpha-01.md
│   └── README.md
└── src/
    ├── cli/
    │   └── ps3_cli.cpp
    ├── core/                  # Kernel, loaders, VFS, HLE
    │   ├── loaders/
    │   │   └── elf_loader.h/.cpp
    │   ├── fs/
    │   │   ├── vfs.h/.cpp
    │   │   ├── psarc_parser.h/.cpp
    │   │   └── sfo_parser.h/.cpp
    │   ├── memory.h/.cpp
    │   ├── hle_kernel.h/.cpp
    │   ├── hle_kernel_syscalls.inl
    │   ├── hle_kernel_internal.h
    │   ├── hle_log.cpp
    │   ├── hle_hw_progress.cpp
    │   ├── hle_registry.h
    │   ├── hle_stage.h
    │   ├── hle_status.h
    │   ├── lv2_syscalls.inl
    │   ├── nid_table.h
    │   ├── assets.cpp
    │   ├── project.h/.cpp
    │   ├── ps3core.h
    │   ├── ps3core_api.cpp
    │   └── ps3_util.h/.c
    ├── processor/
    │   ├── PPU/
    │   │   ├── ppu_lifter.h/.cpp
    │   │   ├── ppu_interpreter.h/.cpp
    │   │   ├── ppu_jit.h/.cpp          # FUTURE dynamic JIT
    │   │   ├── ppu_disasm.h/.cpp
    │   │   └── ppu_runtime*.h
    │   └── SPU/
    │       ├── spu_interpreter.h/.cpp
    │       ├── spu_jit.h/.cpp            # FUTURE
    │       └── spu_local_store.h/.cpp
    ├── RSX/
    │   ├── rsx_backend_interface.h
    │   ├── rsx_device.h/.cpp             # Active RSX body (ex-rsx_stub)
    │   ├── rsx_fifo.h/.cpp
    │   ├── rsx_state.h/.cpp
    │   ├── rsx_memory.h/.cpp
    │   ├── rsx_texture.h/.cpp
    │   ├── rsx_framebuffer.h/.cpp
    │   ├── rsx_shader_decoder.h/.cpp
    │   ├── rsx_shader_compiler.h/.cpp
    │   └── Backends/
    │       ├── DX11/                     # PHASE 1 DEFAULT
    │       ├── OpenGL/                   # PHASE 2 FUTURE
    │       └── Vulkan/                   # PHASE 3 FUTURE
    ├── games-stuff/
    │   ├── Audio/
    │   │   ├── Drivers/                  # xaudio2 default; sdl2/cubeb future
    │   │   ├── Decoders/
    │   │   ├── audio_device.h/.cpp
    │   │   ├── audio_aub.h/.cpp
    │   │   ├── libaudio.h/.cpp
    │   │   ├── audio_mixer.h/.cpp
    │   │   ├── audio_voice.h/.cpp
    │   │   └── audio_dsp.h/.cpp
    │   └── Video/
    │       ├── Decoders/
    │       ├── video_pamf.h/.cpp
    │       ├── video_demuxer.h/.cpp
    │       ├── video_color_space.h/.cpp
    │       ├── video_presenter.h/.cpp
    │       └── video_clock.h/.cpp
    └── UI/                               # WinForms GUI (active)
        ├── panels/                       # FUTURE C++ panels
        ├── Ui/
        ├── *.cs
        └── PS3Recomp.Gui.csproj
```

## Notes

- No top-level `runtime/` **source** tree. Built `ps3rt.dll` + headers still land in `build/dist/runtime/`.
- OpenGL/Vulkan files are **future** stubs (comments in-file).
- Keep at repo root: `LICENSE`, `CODE_OF_CONDUCT.md`, `CONTRIBUTING.md`, `SECURITY.md`.
- Removed from tree policy: root clutter docs (`SUPPORT.md`), optional `scripts/` and thin `tests/` unless reintroduced with CI value.
