# PPSX33 Compiler Setup

PPSX33 uses CMake/Ninja for the host build, the MSVC toolchain for its Windows output path, and the .NET 8 SDK to publish the C# WinForms GUI. The project also supports a C++17 compiler for its native core and CLI.

## 1. Install development tools

For Windows, install Visual Studio 2022 or a newer compatible Visual Studio version with:

- **Desktop development with C++**
- MSVC x64/x86 build tools
- A Windows SDK
- CMake and Ninja
- .NET 8 SDK

Official sources:

- [Visual Studio](https://visualstudio.microsoft.com/downloads/)
- [C++ workload](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-community)
- [CMake](https://cmake.org/)
- [Ninja](https://github.com/ninja-build/ninja)
- [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0)

## 2. Verify the environment

Open a Visual Studio developer command prompt and run:

```bat
where cl
where link
where cmake
where ninja
where dotnet
```

The project currently targets `net8.0-windows`. CMake warns and skips the GUI target when it cannot find `dotnet`.

## 3. Optional local tool staging

The project build copies `Compilers-files/` into `build/dist/Compilers-files/` when the directory exists. The native build driver accepts this folder as the compiler-tool path. It may use local compiler tools such as `cl.exe`, `link.exe`, `ninja.exe`, and associated support files, depending on the configured build path.

To stage tools locally, copy only files you are permitted to use and keep each compiler toolset together with its matching support files. Do not mix binaries from different MSVC versions. The local staging folder is not a substitute for a correctly configured Visual Studio environment or Windows SDK.

Example installed MSVC location:

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<toolset-version>\bin\Hostx64\x64\
```

The edition may be Community, Professional, Enterprise, or BuildTools. Newer releases may use a different versioned directory.

## 4. Build

From the repository root:

```bat
scripts\build_all.bat
```

The build artifacts are placed in `build\dist\`. The GUI is `build\dist\PS3Recomp.exe`, when the .NET GUI target is available. The CLI is `build\dist\ps3_cli.exe`.

For an individual project, the current Windows build path can produce `game.exe`, `ps3rt.dll`, and `guest_image.bin`. Artifact generation is not proof of correct guest execution.

## 5. Licensing and redistribution

Visual Studio/MSVC are Microsoft software governed by their applicable terms. Do not commit proprietary compiler binaries or publish them in releases unless redistribution is explicitly permitted. Users should install the toolchain from Microsoft and check current license terms before sharing any staged files.

See [THIRD_PARTY.md](../THIRD_PARTY.md) for dependency notes.
