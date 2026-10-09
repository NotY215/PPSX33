# PPSX33 Compiler Setup

PPSX33 uses CMake and Ninja for native builds, MSVC for the primary Windows output path, and the .NET 8 SDK for the C# WinForms GUI. The native core and CLI require a C++17-capable compiler.

## 1. Install tools

For Windows, install Visual Studio 2022 or a compatible newer version with:

- **Desktop development with C++**
- MSVC x64/x86 build tools
- Windows SDK
- CMake and Ninja
- .NET 8 SDK

Official sources:

- [Visual Studio](https://visualstudio.microsoft.com/downloads/)
- [C++ workload](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-community)
- [CMake](https://cmake.org/)
- [Ninja](https://github.com/ninja-build/ninja)
- [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0)

## 2. Verify tool discovery

Open a Visual Studio developer command prompt and run:

```bat
where cl
where link
where cmake
where ninja
where dotnet
```

The GUI targets `net8.0-windows`. CMake may skip the GUI target when the .NET SDK is unavailable.

## 3. Local compiler staging

When present, `Compilers-files/` is copied into `build/dist/Compilers-files/`. The build driver accepts this directory as the compiler-tool path. Depending on the build path, tools may include `cl.exe`, `link.exe`, `ninja.exe`, and matching support files.

Keep each toolset and its support files together. Do not mix binaries from different MSVC versions. Local staging does not replace a correctly configured Visual Studio environment or Windows SDK.

Example MSVC path:

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<toolset-version>\bin\Hostx64\x64\
```

The edition may be Community, Professional, Enterprise, or BuildTools. Versioned directories can differ in newer releases.

## 4. Build and output

From the repository root:

```bat
scripts\build_all.bat
```

Output is placed in `build\dist\`. Expected frontends include `PS3Recomp.exe` when the GUI target is available and `ps3_cli.exe` for the CLI.

A project build may emit `game.exe`, `ps3rt.dll`, and `guest_image.bin`. Artifact generation does not prove correct guest execution.

## 5. Licensing

MSVC and Visual Studio are governed by Microsoft's applicable terms. Do not commit or redistribute proprietary compiler binaries unless redistribution is explicitly permitted. See [THIRD_PARTY.md](../THIRD_PARTY.md).
