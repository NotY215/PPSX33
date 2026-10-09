# Compilers-files

This folder is **copied next to the released PS3Recomp.exe**.

When a user clicks **"3. Build exe + dll"** in the GUI, the recompiler
looks here for the tools that turn the generated C++ into a native
`game.exe` + `ps3rt.dll`. That way the end-user does **not** need to
install any C++ toolchain (Visual Studio, MinGW, etc.).

## What to put here (for a self-contained release)

| File / folder | Required? | Notes |
|---------------|-----------|-------|
| `g++.exe` (MinGW-w64 x86-64) | **Yes** (default compiler) | Also copy the matching runtime DLLs (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, …) and ideally the whole MinGW `bin` + needed `lib` / `include` tree. |
| `ninja.exe` | Recommended | Makes the per-game build much faster. If missing the driver falls back to sequential compilation. |
| `c++.exe` | Optional | Used only if `g++.exe` is absent. |
| `cl.exe` + MSVC tools | Future | Planned MSVC backend (ROADMAP Phase 6). Not wired yet. |

## Lookup order (see `src/core/project.cpp`)

1. `Compilers-files/<tool>.exe`  (this folder)
2. System `PATH`

## Important

- **Do not put any .NET / SDK files here.** The GUI itself is built with a normal system-wide .NET 8 SDK. Only the *game* compilation uses this folder.
- These binaries are **never committed to git** (see root `.gitignore`).
- For development you can leave this folder empty and just have `g++` / `ninja` on your PATH.
