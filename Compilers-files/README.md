# PPSX33 Compiler Files

This directory contains optional third-party toolchain files used by PPSX33 during the per-game native build step. The repository currently includes an MSVC tool subset under `MSCV/`, including `cl.exe`, `link.exe`, and supporting DLLs. These are Microsoft-owned components, not PPSX33 code. Review `../THIRD_PARTY.md` and Microsoft's applicable license terms before redistributing them.

## Recommended setup (you have Visual Studio Community)

1. Install Visual Studio or Build Tools with the **Desktop development with C++** workload and a Windows SDK so the compiler environment and libraries are available.
2. The repository has a subset of MSVC tools in `MSCV/`; keep a matching licensed Visual Studio installation for the remaining headers, libraries, SDK files, and environment setup. Do not treat this folder alone as a complete redistributable MSVC installation.
3. If setting up manually, use the matching MSVC tools bin folder layout:

```
Compilers-files/
  MSCV/          <-- or MSVC/
    cl.exe
    link.exe
    ... (contents of VC/Tools/MSVC/<ver>/bin/Hostx64/x64)
```

The build driver uses the MSVC compiler/linker path when available, relying on the Visual Studio environment for INCLUDE/LIB/Windows SDK settings. It prefers `cl.exe` under `Compilers-files/MSCV` or `Compilers-files/MSVC`, otherwise it can use the installed Visual Studio compiler.

**Current validation:** MSVC successfully builds `game.exe` and `ps3rt.dll`. Launching the output currently starts execution but then leaves the recompiled guest-code range, so runtime correctness is still under investigation.

You do **not** need MinGW.

## What you can delete

From a Hostx64/x64 dump you mainly need:

- `cl.exe`, `c1.dll`, `c1xx.dll`, `c2.dll`
- `link.exe`, `mspdbcore.dll` / `mspdb*.dll`
- Related MSVC runtime DLLs that cl/link need

You can remove UI tools, analyzers, and other extras if you want a smaller folder, provided the toolchain still works. Keep Microsoft's license and redistribution terms in mind; do not redistribute files unless permitted.
Keep whatever `cl.exe` fails on when missing (it will name the DLL).

## MinGW fallback

Only if MSVC is completely unavailable. Needs a full MinGW-w64 tree
(including `libexec/.../cc1plus.exe`), not a lone `g++.exe`.

## .NET

Do not put .NET SDK files here.
