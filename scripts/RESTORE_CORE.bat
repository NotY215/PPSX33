@echo off
REM Restores broken core files from last good revision and applies XX-H / packaging patches.
cd /d "%~dp0\.."
echo === Restoring project.cpp and ps3rt.cpp from 41c435a ===
git fetch origin
git show 41c435a501e39f451ab008d1f8070049e248684f:src/core/project.cpp > src\core\project.cpp
git show 41c435a501e39f451ab008d1f8070049e248684f:runtime/ps3rt.cpp > runtime\ps3rt.cpp
echo === Applying patches ===
git apply --whitespace=nowarn scripts\patches\001-ps3rt-focus-assets.patch
git apply --whitespace=nowarn scripts\patches\002-project-game-exe-name.patch
git apply --whitespace=nowarn scripts\patches\003-rsx-icon0.patch
echo === Done. Rebuild All, then game Build. ===
pause
