PPSX33 — Redistributable setup (Windows x64)
============================================

This package does NOT include Visual Studio, MSVC toolchains, or any
bundled compiler under Compilers-files. To rebuild / recompile a game
project from the GUI (Decompile → Build), install Microsoft tooling
on the host PC as described below.

----------------------------------------------------------------------
1) Install Visual Studio Community (required for recompile)
----------------------------------------------------------------------

Download and run the installer shipped with this redist folder:

  VisualStudioCommunitySetup.exe
  (or the current VS Community bootstrapper from Microsoft)

  Official: https://visualstudio.microsoft.com/downloads/

During setup, select at least these workloads / components:

  [x] Desktop development with C++
      - MSVC v143 (or latest) VC++ x64/x86 build tools
      - Windows 10/11 SDK (10.0.19041.0 or newer)
      - C++ CMake tools for Windows   (optional but useful)
      - C++ AddressSanitizer          (optional)

You do NOT need:
  - .NET desktop development   (only if you rebuild the PPSX33 GUI from source)
  - Game development with C++
  - Linux / mobile workloads

Minimum practical install size is large (several GB). Use "Individual
components" if you need a smaller set: MSVC toolset + Windows SDK.

----------------------------------------------------------------------
2) Verify the compiler is on PATH (Developer environment)
----------------------------------------------------------------------

Option A — recommended (no permanent PATH change)
  Start → "x64 Native Tools Command Prompt for VS 2022" (or VS 2026)
  Or run once per shell:

    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

  (Adjust year/edition if your install path differs, e.g. 18\Community.)

  Then:

    where cl
    cl

  You should see cl.exe under ...\VC\Tools\MSVC\...\bin\Hostx64\x64\

Option B — system environment variables (optional)
  Settings → System → About → Advanced system settings → Environment Variables

  User or System PATH — append (example paths; use your actual install):

    C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<version>\bin\Hostx64\x64
    C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools
    C:\Program Files (x86)\Windows Kits\10\bin\<sdkversion>\x64

  Also set (if tools require them):

    INCLUDE = Windows Kits\10\Include\<ver>\um;...\ucrt; MSVC\include; ...
    LIB     = Windows Kits\10\Lib\<ver>\um\x64;...\ucrt\x64; MSVC\lib\x64

  Prefer Option A (vcvars64.bat). Manual INCLUDE/LIB is easy to get wrong.

----------------------------------------------------------------------
3) Visual C++ Redistributable (run generated games)
----------------------------------------------------------------------

Generated game EXEs link against the MSVC runtime. Install the current
x64 redistributable if the game fails with missing VCRUNTIME / MSVCP DLLs:

  https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist

----------------------------------------------------------------------
4) What is NOT shipped in this redist
----------------------------------------------------------------------

  - No MSVC / cl.exe tree under Compilers-files
  - No full Windows SDK copy
  - No proprietary keys or game assets

PPSX33 build scripts detect Visual Studio via vswhere and call
vcvars64.bat. That is expected.

----------------------------------------------------------------------
5) Workflow after install
----------------------------------------------------------------------

  1. Install VS Community with "Desktop development with C++"
  2. Open PPSX33 (PS3Recomp.exe)
  3. Load / decrypt ELF → Decompile → Build
  4. Run the EXE under the project output folder

Successful compile does not mean the title is playable.

----------------------------------------------------------------------
6) Troubleshooting
----------------------------------------------------------------------

  "cl is not recognized"
    → Run from x64 Native Tools prompt, or call vcvars64.bat first.

  Build uses Community cl.exe instead of a local Compilers-files path
    → Expected. Compilers are not redistributed; VS is the toolchain.

  Missing Windows.h / kernel32.lib
    → Install a Windows 10/11 SDK component from the VS installer.


----------------------------------------------------------------------
3) Microsoft Edge WebView2 Runtime (GUI controller mapping page)
----------------------------------------------------------------------

The Controller tab uses the WebView2 control. If the mapping page is blank
or fails to load, install the Evergreen WebView2 Runtime:

  https://developer.microsoft.com/en-in/microsoft-edge/webview2?ch=1&form=MA13LH&cs=2885428259#download

Prefer "Evergreen Bootstrapper" or "Evergreen Standalone Installer" (x64).
After install, restart PPSX33.

Note: Visual Studio and WebView2 installs require network access.
