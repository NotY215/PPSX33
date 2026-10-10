# PPSX33 Alpha 01

**Release title:** PPSX33 Alpha 01 | Static Recompiler, GUI and CLI Preview

**Release channel:** Alpha / early evaluation  
**Platform:** Windows x64 package  
**Status:** Experimental

## Overview

PPSX33 Alpha 01 is an early evaluation build of the PPSX33 PlayStation 3 static recompiler. It packages the WinForms GUI, native core library, command-line frontend, .NET application metadata, runtime source components, and build-tool staging directory in one folder.

This release is intended for developers and technically experienced testers who want to evaluate the current ELF analysis, PPU lifting, project creation, and host build workflow. It is not a complete PlayStation 3 emulator and is not presented as a release capable of reliably running commercial games.

## Package contents

```text
PPSX33-Alpha-01/
  ppsx33-logo.ico
  ppsx33-logo.png
  ps3core.dll
  ps3core.exp
  ps3core.lib
  PS3Recomp.deps.json
  PS3Recomp.dll
  PS3Recomp.exe
  PS3Recomp.pdb
  PS3Recomp.runtimeconfig.json
  ps3_cli.exe
  Compilers-files/
    cmake.exe
    ninja.exe
    README.md
    MSCV/
  runtime/
    boot_continue.patch
    nid_table.h
    ppu_runtime.h
    ps3rt.cpp
    rsx_stub.cpp
    spu_stub.cpp
```

## Included capabilities

- **ELF analysis:** handles supported ELF64 big-endian PowerPC64 inputs, loadable segments, entry/function descriptors, and heuristic symbol, OPD, PRX/NID, and embedded-SPU discovery.
- **Static PPU lifting:** translates recognized instruction instances into generated C/C++ for the host build pipeline.
- **Desktop workflow:** includes the Windows Forms GUI for project creation, lifting, build invocation, and viewing reports.
- **Command-line workflow:** includes `ps3_cli.exe` for automated or developer-driven project processing.
- **Guest runtime foundations:** includes PPU runtime helpers, early SPU interpreter paths, and partial RSX FIFO/control processing.
- **Analysis records:** repository documentation contains measured static snapshots for GOW3, Minecraft (PS3), Uncharted 2, Demon's Souls, and GTA: San Andreas.

## Important limitations

- PPU instruction semantics are incomplete. Approximate operations and no-op fallbacks may be counted as translated.
- A 100% reported static translation rate for an analyzed input does not demonstrate semantic correctness or game compatibility.
- LV2 and PRX behavior is partial and relies on incomplete HLE coverage and heuristic discovery.
- SPU instruction coverage, vector/floating-point behavior, and SPURS integration remain incomplete.
- RSX support is partial; a complete graphics pipeline and commercial-game rendering compatibility are not established.
- Successful analysis or compilation does not mean a game will boot, render correctly, or be playable.
- The package is intended for Windows. The GUI targets .NET 8 for Windows and may require the .NET 8 Desktop Runtime.
- The presence of `MSCV/` does not guarantee that MSVC tools are included or configured. Any proprietary tool binaries must be redistributed only when their licenses permit it.

## Getting started

1. Extract `PPSX33-Alpha-01` to a writable folder.
2. Start `PS3Recomp.exe`. If it fails to start because the .NET runtime is missing, install the .NET 8 Desktop Runtime for Windows.
3. Use an ELF file you are legally authorized to analyze. The tool expects a supported decrypted ELF64 big-endian PowerPC64 input.
4. Create a project, run the lift workflow, and inspect `lift_report.txt` and `analysis_report.txt`.
5. Try the build workflow only when compatible host compiler/linker tools and Windows SDK components are available.
6. Keep logs and the exact steps for any issue you report.

For command-line usage, run:

```bat
ps3_cli.exe --help
```

The CLI project workflow accepts a root directory, project name, ELF path, runtime directory, and compiler-tools directory. See the repository README for the exact argument order.

## Validation status

This release note documents the intended package and current codebase scope. It does not certify that the final archive has been tested. Before publishing, validate the exact release archive on a clean Windows x64 system, confirm the GUI starts, check CLI help, run the synthetic smoke test, and record any tests that could not be completed.

## Legal and content notes

PPSX33 does not include PlayStation 3 firmware, commercial game executables, decrypted game data, encryption keys, or game assets. Use only input files you are legally authorized to analyze. Third-party components remain the property of their respective authors and are subject to their own licenses.

## Feedback

When reporting a problem, include the Alpha version, Windows version, exact steps, relevant log output, and whether the issue occurs during ELF loading, lifting, host compilation, or runtime execution. Do not attach copyrighted game files or sensitive data to public issues.
