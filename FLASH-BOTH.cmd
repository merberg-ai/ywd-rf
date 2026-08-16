@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\common.cmd
if errorlevel 1 (pause & exit /b 20)

echo YWD-RF FLASH-BOTH
echo -----------------
echo This flashes the same firmware to two selected serial ports.
echo.

set "PORT1="
set "PORT2="
for /f "usebackq delims=" %%P in (`py -3 tools\select_port.py`) do set "PORT1=%%P"
echo First board: !PORT1!
echo.
echo Select the SECOND board:
for /f "usebackq delims=" %%P in (`py -3 tools\select_port.py`) do set "PORT2=%%P"

if not defined PORT1 exit /b 2
if not defined PORT2 exit /b 2
if /I "!PORT1!"=="!PORT2!" (
  echo [ERROR] You selected the same port twice.
  pause
  exit /b 3
)

echo.
echo Building once...
py -3 -m platformio run
if errorlevel 1 (pause & exit /b 1)

echo Flashing !PORT1!...
py -3 -m platformio run -t upload --upload-port "!PORT1!"
if errorlevel 1 (pause & exit /b 1)

echo Flashing !PORT2!...
py -3 -m platformio run -t upload --upload-port "!PORT2!"
if errorlevel 1 (pause & exit /b 1)

echo.
echo [OK] Both boards flashed successfully.
pause
