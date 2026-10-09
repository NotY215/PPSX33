# Compilers-files

The app looks for its build tools **in this folder** (it sits next to the app exe). Nothing in here is committed to git
(see `.gitignore`); copy the files in yourself so the app is self-contained.

| File | Used by | Notes |
|------|---------|-------|
| `ninja.exe` | Phase 6 build driver | Runs the generated `output/build.ninja`. If missing, the driver compiles sequentially. |
| `g++.exe` | Phase 6 build driver | **Default compiler** (MinGW-w64, x86-64). Also copy its runtime DLLs / `lib`, `include` folders (or ship the whole MinGW `bin`, `lib`, `libexec`, `x86_64-w64-mingw32` tree here). |
| `c++.exe` | fallback | Used if `g++.exe` is absent. |
| `cl.exe` (+ MSVC toolset) | planned | Not wired in yet, see ROADMAP.md Phase 6 "MSVC backend". |
| .NET runtime / SDK files | GUI | Needed only if you ship the GUI without a system .NET 8 install (use `dotnet publish --self-contained`). |
| `cmake.exe` | optional | Not required: the driver generates `build.ninja` itself. |

Lookup order in `src/core/project.cpp` (`find_tool`): `Compilers-files/<name>.exe`, then the system `PATH`.
