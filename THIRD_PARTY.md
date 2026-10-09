# PPSX33 Third-Party Software and Assets

PPSX33 is maintained separately from the third-party tools and libraries it may use.

- **Microsoft C/C++ toolchain:** `Compilers-files/MSCV/` contains MSVC compiler/linker files including `cl.exe` and `link.exe`. MSVC is proprietary Microsoft software, not owned by PPSX33 or NotY215. Official source: https://visualstudio.microsoft.com/visual-cpp-build-tools/ . License terms: https://visualstudio.microsoft.com/license-terms/ and the applicable Microsoft Visual Studio license. Check redistribution rights for each bundled file and version before publishing or redistributing binaries.
- **Build prerequisites:** CMake: https://cmake.org/ (source: https://github.com/Kitware/CMake; license: https://cmake.org/licensing/). .NET: https://dotnet.microsoft.com/ (source: https://github.com/dotnet; license: https://github.com/dotnet/runtime/blob/main/LICENSE.TXT). These are external prerequisites; do not assume their full installations are included in this repository.
- **RPCS3:** Referenced as an external tool for workflows involving a decrypted ELF. RPCS3 is a separate project with its own license and terms: https://rpcs3.net/ and https://github.com/RPCS3/rpcs3.
- **Sony and PlayStation materials:** PlayStation, PS3, Cell, RSX, and related marks or technologies belong to their respective owners. This repository is not affiliated with or endorsed by Sony Interactive Entertainment.
- **Game files:** This project does not grant rights to commercial games, firmware, keys, decrypted executables, or other copyrighted materials. Do not commit or redistribute those materials. Use only files you own and are authorized to use.

PPSX33 and NotY215 do not claim ownership or copyright over Microsoft, RPCS3, CMake, .NET, PlayStation technology, or any other third-party component. Each component remains subject to its own terms. This file is an attribution and dependency notice, not a grant of redistribution rights. Review the applicable license for the exact version before redistributing bundled files.
