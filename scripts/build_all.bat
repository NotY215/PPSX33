@echo off
REM Build native core + GUI into build\dist (uses system VS for game rebuilds)
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
echo   Run:  build\dist\PS3Recomp.exe
echo   VS setup: build\dist\redist\README.txt
echo ============================================
exit /b 0
