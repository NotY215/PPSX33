@echo off
REM Builds ps3core.dll + ps3_cli.exe (CMake/Ninja) and the C# GUI into build\dist
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build || exit /b 1
dotnet publish src\gui -c Release -o build\dist || exit /b 1
echo Done. Put cl.exe/g++.exe/c++.exe/ninja.exe etc. into build\dist\Compilers-files (see Compilers-files\README.md)
