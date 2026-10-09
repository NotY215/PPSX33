# PPSX33 Third-Party Software and Assets

PPSX33 uses or references third-party tools, frameworks, and platform interfaces. Their authors and rights holders retain ownership of their respective components.

| Component | Role in this repository | Official source | License / terms |
| --- | --- | --- | --- |
| Microsoft Visual Studio / MSVC | Windows host compiler and linker | https://visualstudio.microsoft.com/downloads/ | https://visualstudio.microsoft.com/license-terms/ |
| CMake | Host build configuration | https://cmake.org/ | https://cmake.org/licensing/ |
| Ninja | Build execution | https://github.com/ninja-build/ninja | https://github.com/ninja-build/ninja/blob/master/COPYING |
| .NET 8 | WinForms GUI framework and SDK | https://dotnet.microsoft.com/download/dotnet/8.0 | https://github.com/dotnet/runtime/blob/main/LICENSE.TXT |
| RPCS3 | Optional external tool referenced by the GUI workflow | https://rpcs3.net/ | https://github.com/RPCS3/rpcs3/blob/master/LICENSE |
| PSL1GHT | Optional toolchain/API used to build the PS3 Pong sample | https://github.com/ps3dev/PSL1GHT | Review the repository license and bundled component licenses |

PPSX33 does not claim ownership of these tools, frameworks, APIs, trademarks, or third-party materials. Listing a tool does not imply endorsement or a bundled dependency.

## Platform and game content

PlayStation, PS3, Cell, RSX, and related marks and technologies belong to their respective rights holders. PPSX33 is an independent project and is not affiliated with or endorsed by Sony Interactive Entertainment.

PPSX33 does not provide rights to commercial games, firmware, encryption keys, decrypted executables, or copyrighted game materials. Users are responsible for ensuring they have the legal right to access and process any files they use.

## Compiler files

The `Compilers-files/` directory is a local tool-staging location copied into the build output when present. Microsoft compiler binaries and related files remain subject to Microsoft's terms. A successful local build does not mean the staged files may be redistributed. Install tools from official sources and check the terms for the exact edition and toolset.

NotY215 and PPSX33 do not claim ownership of third-party software, trademarks, or copyrighted assets referenced here.
