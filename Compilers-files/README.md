# Compilers-files

This folder is copied next to the released PS3Recomp.exe.

When a user clicks "3. Build exe + dll" the recompiler looks here for the
tools that turn generated C++ into native game.exe + ps3rt.dll. That way the
end-user does not need a system-wide C++ toolchain.

## CRITICAL: do not put only g++.exe here

`g++.exe` is just a driver. It needs the rest of the MinGW-w64 toolchain:

```
Compilers-files/
  g++.exe
  gcc.exe
  c++.exe          (optional)
  ninja.exe
  libgcc_s_seh-1.dll
  libstdc++-6.dll
  libwinpthread-1.dll
  ... other MinGW bin DLLs ...

  libexec/
    gcc/
      x86_64-w64-mingw32/
        <version>/
          cc1plus.exe      <-- this is what was missing
          cc1.exe
          collect2.exe
          ...

  lib/
  include/
  x86_64-w64-mingw32/
```

Easiest correct setup:

1. Install MinGW-w64 (WinLibs, MSYS2, or official).
2. Copy the entire `bin` folder contents into Compilers-files/.
3. Also copy `libexec`, `lib`, `include`, and `x86_64-w64-mingw32` folders
   next to them (same relative layout as a normal MinGW install).
4. Put `ninja.exe` in Compilers-files/ as well.

If the layout is wrong you will see:
```
g++.exe: fatal error: cannot execute 'cc1plus': CreateProcess: No such file or directory
```

## Fallback

If Compilers-files is empty or broken, the build driver falls back to whatever
`g++` / `ninja` is on the system PATH.

## .NET

Do not put any .NET SDK files here. The GUI is built with a normal system
.NET 8 SDK. Only the per-game C++ compile uses this folder.
