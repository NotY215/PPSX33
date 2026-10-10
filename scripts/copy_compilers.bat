@echo off
rem Usage: copy_compilers.bat <src> <dst>
rem Always exits 0 so CMake/ninja never fail on locked files.
set "SRC=%~1"
set "DST=%~2"
if "%SRC%"=="" exit /b 0
if "%DST%"=="" exit /b 0
if not exist "%DST%" mkdir "%DST%" 2>nul
where robocopy >nul 2>&1
if errorlevel 1 goto :xcopy_fallback
robocopy "%SRC%" "%DST%" /E /XO /NFL /NDL /NJH /NJS /nc /ns /np >nul 2>&1
exit /b 0
:xcopy_fallback
xcopy /E /I /Y /Q "%SRC%" "%DST%\" >nul 2>&1
exit /b 0
