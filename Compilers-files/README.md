# Compilers-files — local toolchain staging

This directory is copied into `build/dist/Compilers-files/` when you build PPSX33.
The **game project build** (`build_msvc.bat` from `project.cpp`) prefers tools from here so end users do not need a full Visual Studio install on `PATH`.

## Required layout for developers

After installing Visual Studio (or Build Tools) with **Desktop development with C++**, **copy** the MSVC host tools into this tree:

```text
Compilers-files/
  README.md                 (this file)
  MSCV/                     ← preferred name (also accepts MSVC / msvc / mscv)
    cl.exe
    link.exe
    lib.exe                 (optional)
    *.dll                   (MSVC support DLLs next to cl.exe)
  ninja/                    (optional)
    ninja.exe
  mingw64/                  (optional g++ fallback)
    bin/g++.exe
```

### How to populate `MSCV/`

1. Install Visual Studio 2022/2026 (or Build Tools) with the C++ workload and a Windows SDK.
2. Locate the host x64 tools, for example:

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<ver>\bin\Hostx64\x64\
```

3. Copy **at least** `cl.exe`, `link.exe`, and the supporting DLLs from that folder into:

```text
Compilers-files/MSCV/
```

4. Rebuild PPSX33 so the folder is staged under `build/dist/Compilers-files/MSCV/`.

The build driver searches, in order: `MSCV`, `MSVC`, `msvc`, `mscv`, then the root of `Compilers-files`.

### Windows SDK (D3D headers / libs)

`cl.exe` alone is not enough to compile `rsx_stub.cpp` (D3D10/11).
`project.cpp` still **calls `vcvars64.bat`** (when found) only to set `INCLUDE` / `LIB` for the Windows SDK. You do **not** need those SDK folders inside `Compilers-files`, but a machine build still needs a Windows SDK installed via Visual Studio Installer.

Optional env: `PS3RT_GFX=D3D10` or `D3D11`.

## End-user package (`build/dist`)

```text
build/dist/
  PS3Recomp.exe
  ps3core.dll
  runtime/
  Compilers-files/
    MSCV/cl.exe
    MSCV/link.exe
  assets/
  projects/         (created at runtime)
```

Developers compiling from source must keep `Compilers-files/MSCV` populated so local builds match the dist layout.

## Install tools (host build of PPSX33 itself)

- Visual Studio with **Desktop development with C++**
- Windows SDK
- CMake, Ninja
- .NET 8 SDK (GUI)

```bat
where cl
where link
where cmake
where ninja
where dotnet
scripts\build_all.bat
```

## Licensing

Do not commit proprietary MSVC binaries unless redistribution is allowed. Prefer documenting the copy steps. See [THIRD_PARTY.md](../THIRD_PARTY.md).

## Verify game builds use staged tools

Log should show:

```text
Using Compilers-files cl: ...\Compilers-files\MSCV\cl.exe
```
