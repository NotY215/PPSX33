# Compilers-files

Optional support files for the per-game native build step.

## Recommended setup (you have Visual Studio Community)

1. Install workload **Desktop development with C++** (so `vcvars64.bat` exists).
2. Optionally copy the MSVC tools bin folder here:

```
Compilers-files/
  MSCV/          <-- or MSVC/
    cl.exe
    link.exe
    ... (contents of VC/Tools/MSVC/<ver>/bin/Hostx64/x64)
```

The build driver:
- Calls `vcvars64.bat` for INCLUDE / LIB / Windows SDK
- Prefers `cl.exe` from `Compilers-files/MSCV` or `Compilers-files/MSVC` if present
- Otherwise uses the `cl` from the Visual Studio install

You do **not** need MinGW.

## What you can delete

From a Hostx64/x64 dump you mainly need:

- `cl.exe`, `c1.dll`, `c1xx.dll`, `c2.dll`
- `link.exe`, `mspdbcore.dll` / `mspdb*.dll`
- Related MSVC runtime DLLs that cl/link need

You can remove UI tools, analyzers, and other extras if you want a smaller folder.
Keep whatever `cl.exe` fails on when missing (it will name the DLL).

## MinGW fallback

Only if MSVC is completely unavailable. Needs a full MinGW-w64 tree
(including `libexec/.../cc1plus.exe`), not a lone `g++.exe`.

## .NET

Do not put .NET SDK files here.
