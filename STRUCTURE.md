# PPSX33 Repository Structure

**Website:** https://ppsx33.xo.je/  
**Last updated:** 2026-10-29

This document is the canonical map of the repository after the modular `src/` restructure.  
All runtime code lives under `src/`. There is no `runtime/` tree.

```
PPSX33/
├── CMakeLists.txt
├── PS3Recomp.sln                 # GUI → src/UI/PS3Recomp.Gui.csproj
├── README.md / ROADMAP.md / STRUCTURE.md / ENGINEERING.md / USER_MANUAL.md
├── LICENSE / CODE_OF_CONDUCT.md / CONTRIBUTING.md / SECURITY.md
├── assets/
├── docs/                         # architecture, legal/, games/
├── redist/ / releases/
└── src/
    ├── core/
    │   ├── loaders/
    │   ├── fs/
    │   ├── hle_kernel.cpp        # owns ps3rt_log + lifecycle
    │   ├── hle_kernel_syscalls.inl  # owns ps3rt_progress_hw + GCM HLE
    │   ├── hle_log.cpp              # Optional/reference only — ps3rt_log lives in hle_kernel.cpp (do not dual-link)
    │   ├── hle_hw_progress.cpp      # Optional/reference only — ps3rt_progress_hw lives in hle_kernel_syscalls.inl
    │   └── ...
    ├── processor/PPU|SPU/
    ├── RSX/
    ├── games-stuff/Audio|Video/
    ├── UI/
    └── cli/
```

See full tree in repository history / prior STRUCTURE revisions for every file leaf.

**Link note:** CMake `ps3rt` must not list both `hle_kernel.cpp` and `hle_log.cpp` / `hle_hw_progress.cpp` together (LNK2005).
