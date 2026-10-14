# Restore broken core files + apply packaging / XX-H helper patches

If `src/core/project.cpp` or `runtime/ps3rt.cpp` on `main` is broken (PLACEHOLDER / truncated), run:

```bat
cd /d E:\PPSX33
git pull origin main
scripts\RESTORE_CORE.bat
```

Or manually:

```bat
git show 41c435a501e39f451ab008d1f8070049e248684f:src/core/project.cpp > src\core\project.cpp
git show 41c435a501e39f451ab008d1f8070049e248684f:runtime/ps3rt.cpp > runtime\ps3rt.cpp
git apply scripts\patches\001-ps3rt-focus-assets.patch
git apply scripts\patches\002-project-game-exe-name.patch
git apply scripts\patches\003-rsx-icon0.patch
```

Then rebuild **All** (core + GUI), Decompile, Build the game, **Copy to game folder** (files go to `PS3_GAME\USRDIR` next to EBOOT.BIN), run the game-named EXE from USRDIR.

## What the patches do

1. **ps3rt** — pad/XInput log only when the game window is focused; call `ps3rt_assets_boot_scan()` at boot.
2. **project** — output EXE named after the project folder (e.g. `God of War III.exe`).
3. **rsx** — load `PS3_GAME/ICON0.PNG` for the **window and taskbar** icon (not the .exe file icon on disk).

## XX-H

Full title-to-ingame playable pass remains the open research goal. Correct USRDIR placement + asset scan are required packaging steps; solid blue without `[fs]`/`[psarc]` lines still means guest control flow has not reached cellFs/draws.
