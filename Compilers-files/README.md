# PPSX33 Compiler Setup

PPSX33 uses the Microsoft C/C++ compiler and linker to build the generated Windows executable and runtime DLL. The recommended toolchain is the latest stable **Visual Studio 2026 (version 18)** installation with the C++ desktop workload.

## 1. Install the toolchain

1. Download Visual Studio 2026 from https://visualstudio.microsoft.com/downloads/.
2. In Visual Studio Installer, select **Desktop development with C++**.
3. Include the latest MSVC x64/x86 build tools and a Windows 11 SDK.
4. Install the **.NET desktop development** workload and the .NET targeting pack required by the project. The GUI project currently targets `net8.0-windows`.
5. Install the latest CMake and Ninja versions if they are not included in the selected Visual Studio components.

Official references:
- Visual Studio 2026: https://visualstudio.microsoft.com/downloads/
- C++ workload: https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-community
- .NET installation and Visual Studio compatibility: https://learn.microsoft.com/en-us/dotnet/core/install/windows

## 2. Populate this folder for the local build

Open **x64 Native Tools Command Prompt for VS 2026** from the Start menu and run:

```bat
where cl
where link
where cmake
where ninja
where dotnet
```

The MSVC compiler tools are installed in a versioned directory similar to:

```text
C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\<toolset-version>\bin\Hostx64\x64\
```

The edition directory may be `Community`, `Professional`, `Enterprise`, or `BuildTools`, depending on the installation.

For a local tool staging setup, create `Compilers-files\MSCV\` and copy the **complete contents** of the matching `Hostx64\x64\` directory into it. Keep the compiler and linker together with their matching support files, including files such as `cl.exe`, `link.exe`, `c1.dll`, `c1xx.dll`, `c2.dll`, and the matching PDB/runtime support DLLs. The exact filenames vary by toolset version, so use the contents installed by Visual Studio rather than mixing files from different versions.

The compiler directory alone does not provide the Windows SDK headers/libraries or the full Visual Studio build environment. Run the build from a Visual Studio developer environment so the matching INCLUDE, LIB, SDK, and tool paths are configured.

## 3. Build PPSX33

From the repository root, run:

```bat
scripts\build_all.bat
```

The build output is placed in `build\dist\`. The GUI executable is `build\dist\PS3Recomp.exe`.

For an individual recompiled game, the current MSVC path can emit `game.exe` and `ps3rt.dll`. Successful compilation confirms that the artifacts were produced; it does not confirm that the guest program runs correctly. Current runtime validation shows the program starts and then leaves the recompiled guest-code range.

## 4. Licensing

MSVC and Visual Studio files are Microsoft software governed by Microsoft's license terms. Install and use Visual Studio under its applicable license. Before sharing a public repository or release, review Microsoft's redistribution terms and the licenses for every included file. Do not publish proprietary compiler binaries in this repository unless the applicable terms explicitly permit that distribution.

See [THIRD_PARTY.md](../THIRD_PARTY.md) for dependency and licensing information.
