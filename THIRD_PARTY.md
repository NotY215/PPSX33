# PPSX33 Third-Party Software and Assets

PPSX33 uses or references third-party tools, frameworks, and platform interfaces. Their authors and rights holders retain ownership of their respective components.

| Component | Role | Official source | License or terms |
| --- | --- | --- | --- |
| Microsoft Visual Studio / MSVC | Windows host compiler and linker | https://visualstudio.microsoft.com/downloads/ | https://visualstudio.microsoft.com/license-terms/ |
| CMake | Host build configuration | https://cmake.org/ | https://cmake.org/licensing/ |
| Ninja | Build execution | https://github.com/ninja-build/ninja | https://github.com/ninja-build/ninja/blob/master/COPYING |
| .NET 8 | WinForms GUI framework and SDK | https://dotnet.microsoft.com/download/dotnet/8.0 | https://github.com/dotnet/runtime/blob/main/LICENSE.TXT |
| RPCS3 | Optional external tool referenced by the GUI workflow | https://rpcs3.net/ | https://github.com/RPCS3/rpcs3/blob/master/LICENSE |
| PSL1GHT | Optional PS3 sample toolchain/API | https://github.com/ps3dev/PSL1GHT | Review repository and bundled component licenses |

Listing a component does not imply endorsement or that the component is bundled. PPSX33 does not claim ownership of third-party tools, frameworks, APIs, trademarks, or materials.

## Platform and game content

PlayStation, PS3, Cell, RSX, and related marks and technologies belong to their respective rights holders. PPSX33 is independent and is not affiliated with or endorsed by Sony Interactive Entertainment.

PPSX33 does not grant rights to commercial games, firmware, encryption keys, decrypted executables, or copyrighted game materials. Developers are responsible for ensuring they are authorized to access and process input files.

## Compiler files

`Compilers-files/` is a local tool-staging location copied into build output when present. Microsoft compiler binaries remain subject to Microsoft's terms. A successful local build does not authorize redistribution. Install tools from official sources and review the terms for the exact edition and toolset.
