@echo off
REM Compile PS3 PPC64 Pong ELF, then lift and recompile it through PPSX33.
setlocal
cd /d "%~dp0.."
if "%PSL1GHT%"=="" (
  echo ERROR: Configure the PS3 homebrew SDK environment and set PSL1GHT.
  exit /b 2
)
where ppu-gcc >nul 2>nul || (echo ERROR: ppu-gcc not found in PATH.& exit /b 2)
if not exist build\dist\ps3_cli.exe (
  echo ERROR: build\dist\ps3_cli.exe not found. Run scripts\build_all.bat first.
  exit /b 2
)
if not exist build\pong mkdir build\pong
echo [1/3] Compile PS3 PPU/VMX source to ELF...
ppu-gcc -O2 -Wall -Wextra -maltivec -mabi=altivec -I"%PSL1GHT%\ppu\include" tests\games\pong\pong.c -L"%PSL1GHT%\ppu\lib" -lrsx -lgcm_sys -lsysutil -lio -lm -o build\pong\pong.elf
if errorlevel 1 exit /b 1
echo [2/3] Lift and build using PPSX33...
build\dist\ps3_cli.exe build\pong\project Pong build\pong\pong.elf runtime Compilers-files
if errorlevel 1 exit /b 1
echo [3/3] Verify generated output...
if exist build\pong\project\Pong\output\game.exe (
  echo Output: build\pong\project\Pong\output\game.exe
) else (
  echo ERROR: native executable not found. Inspect generated lift_report.txt.
  exit /b 1
)
