@echo off
REM Full release-style build for Visual Studio Community / command-line.
REM 1. CMake configures + builds native (ps3core.dll, ps3_cli.exe, runtime/)
REM 2. dotnet builds the GUI into the same build\dist folder

setlocal
cd /d "%~dp0.."

echo === [1/2] Native (CMake) ===
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build
if errorlevel 1 exit /b 1

echo.
echo === [2/2] GUI (.NET) ===
dotnet publish src\gui\PS3Recomp.Gui.csproj -c Release -o build\dist --self-contained false
if errorlevel 1 exit /b 1

echo.
echo ============================================
echo   Build finished.
echo   Output folder:  build\dist\
echo     PS3Recomp.exe   (the UI you run)
echo     ps3core.dll
echo     runtime\
echo     Compilers-files\  (put g++.exe / ninja.exe here for self-contained game builds)
echo ============================================
exit /b 0
