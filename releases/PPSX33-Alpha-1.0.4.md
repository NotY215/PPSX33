# PPSX33 Alpha 1.0.4

**Release title:** PPSX33 Alpha 1.0.4 | Qt Desktop Workflow and Vulkan Runtime Update  
**Release channel:** Alpha / early evaluation  
**Platform:** Windows x64  
**Status:** Experimental  
**Release date:** 2026-11-02

## Overview

PPSX33 Alpha 1.0.4 documents the project's current development direction toward rebuilding PlayStation 3 game code as native Windows executables. Compared with the historical Alpha 01 notes, this release reflects the newer Qt 6 desktop application, modular runtime layout, Vulkan-first presentation path, and more explicit build and validation workflow.

PPSX33 remains an experimental recompiler, not a complete PlayStation 3 emulator. This note describes the current repository direction and is not proof that every listed feature is included in a downloadable archive. Confirm the contents of the exact package you download; a source-tree update does not retroactively update an older binary.

**Project website:** [PPSX33](https://ppsx33.xo.je/)  
**Repository:** [NotY215/PPSX33](https://github.com/NotY215/PPSX33)

## Highlights

- **Updated desktop application:** The current source describes a native C++ / Qt 6 Widgets GUI, with the desktop executable named `PPSX33.exe`.
- **Modular runtime outputs:** The documented build output uses `build/dist/` for the GUI, runtime DLLs, and supporting runtime files instead of the historical Alpha 01 package layout.
- **Vulkan-first presentation:** Vulkan 1.2 is the default host graphics path, including an asynchronous window/presentation flow. OpenGL 4.3 is a secondary path with parity still in progress.
- **Build and project workflow:** The repository documents a host build helper, GUI project flow, PPU lifting, native build orchestration, and CLI tooling.
- **Clearer validation expectations:** Analysis, lifting, compilation, booting, rendering, and playability are treated as separate milestones. Translation percentages are not compatibility ratings.
- **Expanded engineering documentation:** The README links to the roadmap, architecture and diagrams, developer guide, user manual, runtime requirements, PPU coverage notes, and game-specific test records.

## Current pipeline

The intended workflow is:

1. Load a decrypted ELF or supported game layout that you are legally authorized to analyze.
2. Analyse the input and lift supported PPU code into generated C++.
3. Resolve runtime dependencies through the implemented LV2, PRX, NID, and library HLE paths.
4. Build a native Windows executable using the required Microsoft C++ toolchain.
5. Validate the result with logs, traces, and repeatable title-specific tests.

The exact supported inputs and outputs depend on the code and package version being used.

## Runtime and graphics notes

- **Default graphics API:** Vulkan 1.2.
- **Secondary graphics path:** OpenGL 4.3, with feature parity still in progress.
- **Direct3D 11:** The current README says the D3D11 backend and dependencies have been removed.
- **Host requirements:** 64-bit Windows 10 or later and a GPU/driver exposing Vulkan 1.2 or OpenGL 4.3 for the documented presentation baseline.
- **Build requirements:** The development setup uses Visual Studio C++ desktop tools and a Windows SDK, Qt 6, and the CMake/Ninja tools required by the relevant targets.

These requirements only describe the current documented host path. They do not guarantee that a game will boot or render correctly.

## Known limitations

- PPU instruction translation and runtime semantics remain incomplete; recognised or translated instructions are not necessarily correct.
- LV2, PRX, NID, and cell-library coverage remains partial.
- SPU instruction behaviour, MFC/DMA ordering, and SPURS integration are incomplete.
- RSX/GCM support is still in progress. A complete game graphics pipeline, including title-authored commands and shaders, is not established.
- Audio, input, save-data, and other host service paths require title-level verification.
- A successful build, lift report, or opened window does not prove that a game boots, renders correctly, or is playable.
- The exact capabilities of a downloadable archive may differ from the current `main` branch.

## Getting started

1. Download and extract the Alpha 1.0.4 package, if a package is published for this version.
2. Read the package-specific instructions and verify the included files.
3. Use only decrypted ELF/game data you are legally authorized to analyze.
4. Run the available GUI or CLI workflow supported by that package.
5. Review the lift report, analysis report, build output, and runtime logs. Do not infer compatibility from a successful earlier stage.

For building from source, follow the current repository README, developer guide, and toolchain notes. Build commands and output paths can vary with the checked-out source revision and local toolchain.

## Legal and content notes

PPSX33 does not provide PlayStation 3 firmware, commercial game executables, decrypted game data, encryption keys, or game assets. Users are responsible for ensuring they are authorized to use any input files. Third-party components remain subject to their respective licenses. See [LEGAL.md](../LEGAL.md), [TERMS.md](../TERMS.md), and [THIRD_PARTY.md](../THIRD_PARTY.md).

## Reporting issues

Include the PPSX33 version, Windows version, source revision or archive name, exact reproduction steps, and relevant logs. Identify whether the failure occurs during ELF loading, analysis/lifting, native compilation, runtime startup, graphics presentation, or game execution. Do not upload copyrighted game files, firmware, keys, or sensitive data to public issues.

---

**Release status:** This is release documentation. It does not, by itself, mean a binary archive has been built, tested, or published. Check the official project page or an official release entry for an actual download.
