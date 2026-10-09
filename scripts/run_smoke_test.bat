@echo off
REM Windows smoke test: generate tiny ELF -> lift -> build -> run, expect "OK"
setlocal
cd /d "%~dp0.."

if not exist build\dist\ps3_cli.exe (
    echo ERROR: build\dist\ps3_cli.exe not found. Run CMake build first.
    exit /b 1
)

echo [1/4] Generating test ELF...
python tests\make_test_elf.py build\test.elf || exit /b 1

echo [2/4] Creating project + lifting + building...
if exist build\smoke_root rmdir /s /q build\smoke_root
build\dist\ps3_cli.exe build\smoke_root SmokeTest build\test.elf runtime Compilers-files
if errorlevel 1 (
    echo SMOKE TEST FAILED at lift/build stage
    exit /b 1
)

echo [3/4] Running generated game.exe...
cd build\smoke_root\SmokeTest\output
game.exe > ..\..\..\smoke_out.txt 2>&1
cd ..\..\..\..

echo [4/4] Checking output...
findstr /x /c:"OK" build\smoke_out.txt >nul
if errorlevel 1 (
    echo SMOKE TEST FAILED – expected "OK"
    type build\smoke_out.txt
    exit /b 1
)

echo.
echo ==============================
echo   SMOKE TEST PASSED
echo ==============================
exit /b 0
