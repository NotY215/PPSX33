@echo off
REM Single command that builds EVERYTHING (native + GUI) into build\dist
setlocal
cd /d "%~dp0.."

echo === Configuring + Building (CMake) ===
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build build
if errorlevel 1 exit /b 1

echo.
echo ============================================
echo   Build finished.
echo   Run the UI with:
echo     build\dist\PS3Recomp.exe
echo ============================================
exit /b 0
