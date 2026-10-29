# PPSX33 Engineering notes

**Website:** [https://ppsx33.xo.je/](https://ppsx33.xo.je/)  
**Architecture:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)  
**Diagrams:** [docs/DIAGRAMS.md](docs/DIAGRAMS.md)  
**Roadmap:** [ROADMAP.md](ROADMAP.md)

## Host build

- Configure with CMake (x64 Release).
- Artifacts land under `build/dist/`: `PS3Recomp.exe`, `ps3core.dll`, `ps3rt.dll` / `ps3rt.lib`, `ppsx33-logo.ico`.
- GUI targets .NET 8 Windows Forms; runtime and core are C++17 MSVC.

## Game build path

1. Load **decrypted ELF** in the GUI (legal gate required).
2. Decompile / lift → generated C++ under the project `output/src`.
3. Build uses system Visual Studio (`vcvars64` / `cl` / `link`). Redist folder is installer + guide only.
4. Prefer **prebuilt `ps3rt.dll`** so only game objects recompile.
5. **ps3rt link units:** do not dual-link `hle_log.cpp` or `hle_hw_progress.cpp` with `hle_kernel.cpp` (LNK2005 on `ps3rt_log` / `ps3rt_progress_hw`). Logging and `ps3rt_progress_hw` are owned by `hle_kernel.cpp` + `hle_kernel_syscalls.inl`.

## Icons

- Ship **`ppsx33-logo.ico` only** for app EXE, window, taskbar, and UI logo (`Icon.ToBitmap`).
- Optional embed into generated game EXE via `game_icon.rc` when the `.ico` is found next to dist.

## Correctness env

| Variable | Effect |
| --- | --- |
| `PS3RT_STRICT_FAULT=1` | Halt on invalid PC; no invented recovery targets |
| `PS3RT_INJECT_SEED=0` | Disable boot FIFO assist inject |
| `PS3RT_SPU_TRACE=1` | Verbose SPU MFC / tag logs |
