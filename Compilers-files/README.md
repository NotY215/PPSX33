# Compilers-files: packaged toolchain support

This directory contains compiler/build tools staged with PPSX33. In the Alpha 01 distribution, the package layout is:

```text
PPSX33-Alpha-01/
  Compilers-files/
    cmake.exe
    ninja.exe
    README.md
    MSCV/
```

## What these tools are for

- `cmake.exe` and `ninja.exe` support build orchestration.
- `MSCV/` is the designated location for Microsoft C/C++ compiler and linker tools when those tools are available and licensed for redistribution.
- The project build driver may also rely on a compatible Visual Studio installation and Windows SDK, depending on the generated project and selected graphics backend.
- The end-user GUI targets `.NET 8 for Windows`; it is framework-dependent unless the release is explicitly republished as self-contained.

The exact set of tools found in the folder can vary by package. Do not assume that an empty `MSCV/` folder contains a compiler.

## Developer setup

For building PPSX33 from source, install Visual Studio 2022 or a compatible version with **Desktop development with C++**, a Windows SDK, CMake, Ninja, and the .NET 8 SDK. From the repository root, run:

```bat
scripts\build_all.bat
```

Build output is staged under `build\dist\`.

## Redistribution and licensing

Microsoft compiler and linker binaries are proprietary. Include them in a public release only if the applicable Visual Studio / Build Tools license explicitly permits the intended redistribution. Otherwise, ship the empty `MSCV/` directory or instructions for installing the required tools, and do not bundle proprietary binaries. Review [THIRD_PARTY.md](../THIRD_PARTY.md) before distributing the package.

## Alpha package validation

Before publishing, verify that the archive contains the expected files, that `PS3Recomp.exe` starts on a clean supported Windows machine, that `ps3_cli.exe --help` responds, and that the synthetic smoke test completes. Record any skipped test and do not claim that these checks prove commercial-game compatibility.
