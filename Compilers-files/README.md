# Compilers-files

Optional. Used only if Visual Studio / MSVC is not detected.

## Preferred: Visual Studio Community (MSVC)

If you have Visual Studio Community with the **Desktop development with C++**
workload installed, you do **not** need anything in this folder.

The build driver finds `vcvars64.bat` automatically (via vswhere or common
paths) and compiles the game with `cl.exe` + `link.exe`.

Just press **3. Build exe + dll** in the GUI.

## Optional fallback: MinGW-w64

Only needed if MSVC is not installed. In that case put a **complete**
MinGW-w64 tree here (not just g++.exe):

```
Compilers-files/
  g++.exe
  gcc.exe
  ninja.exe
  libgcc_s_seh-1.dll
  libstdc++-6.dll
  libwinpthread-1.dll
  libexec/gcc/.../cc1plus.exe   <-- required
  lib/
  include/
  x86_64-w64-mingw32/
```

A lone g++.exe will be ignored (it cannot find cc1plus).

## .NET

Do not put any .NET files here. The GUI uses a normal system .NET 8 SDK.
