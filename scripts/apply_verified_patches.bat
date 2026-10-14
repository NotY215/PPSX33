@echo off
cd /d "%~dp0\.."
echo Applying verified patches (game-named EXE, ICON0, Roadmap tab)...
git apply --check scripts\verified-patches.patch
if errorlevel 1 (
  echo Patch check failed - already applied or conflict.
  pause
  exit /b 1
)
git apply scripts\verified-patches.patch
if errorlevel 1 (
  echo Apply failed
  pause
  exit /b 1
)
echo.
echo SUCCESS. Review: git diff
echo Then commit and push:
echo   git add src\core\project.cpp src\gui\MainForm.cs runtime\rsx_stub.cpp
echo   git commit -m "Game-named EXE, ICON0 window icon, Roadmap tab"
echo   git push origin main
pause
