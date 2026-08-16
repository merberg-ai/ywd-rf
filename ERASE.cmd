@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\common.cmd
if errorlevel 1 (pause & exit /b 20)

for /f "usebackq delims=" %%P in (`py -3 tools\select_port.py`) do set "PORT=%%P"
if not defined PORT exit /b 2

echo.
echo WARNING: this erases the entire flash on %PORT%.
set /p "ANS=Type ERASE to continue: "
if /I not "%ANS%"=="ERASE" exit /b 0

py -3 -m platformio run -t erase --upload-port "%PORT%"
if errorlevel 1 (
  echo [ERROR] Erase failed.
  pause
  exit /b 1
)
echo [OK] Flash erased.
pause
