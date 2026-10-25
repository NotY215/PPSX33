# PPSX33 Release Redistributables

This document covers **prebuilt PPSX33** launch needs and **game recompile** needs. Compilers are **not** shipped in the package (no `Compilers-files` / MSVC tree).

See also **[redist/README.txt](redist/README.txt)** for a full Visual Studio Community install and PATH / `vcvars64` guide.

## Launch prebuilt PPSX33 (GUI / CLI)

- **Windows 10/11 x64**
- **Microsoft Visual C++ Redistributable (x64)** if native DLLs report missing `VCRUNTIME` / `MSVCP`:
  [Latest supported VC++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
- **.NET:** only if the shipped UI is framework-dependent; prefer self-contained publish so users do not need a separate .NET install.

## Recompile a game (Decompile → Build)

Required on the host:

1. **Visual Studio Community** (bootstrapper may be placed in `redist/` as `VisualStudioCommunitySetup.exe`)
2. Workload: **Desktop development with C++**
   - MSVC v143 (or current) x64/x86 build tools
   - Windows 10/11 SDK
3. Open an **x64 Native Tools** prompt or run `vcvars64.bat` so `cl.exe` is available

PPSX33 discovers VS via `vswhere` and initializes the MSVC environment. Using Community `cl.exe` is expected.

## Not shipped

- MSVC / `cl.exe` under `Compilers-files`
- Full Windows SDK copy
- Proprietary keys or game assets

## Official downloads

- [Visual Studio downloads](https://visualstudio.microsoft.com/downloads/)
- [Windows SDK](https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads)
- [VC++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
