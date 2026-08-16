@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\common.cmd || (pause & exit /b %errorlevel%)

if not "%~1"=="" (
  set "PORT=%~1"
) else (
  for /f "usebackq delims=" %%P in (`py -3 tools\select_port.py`) do set "PORT=%%P"
)

if not defined PORT exit /b 2

echo [YWD-RF] Opening serial monitor on %PORT% at 115200 baud.
echo Press Ctrl+C to exit.
pio device monitor -p "%PORT%" -b 115200
