@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\common.cmd || (pause & exit /b %errorlevel%)

for /f "usebackq delims=" %%P in (`py -3 tools\select_port.py`) do set "PORT=%%P"
if not defined PORT exit /b 2

echo.
echo WARNING: this erases the entire flash on %PORT%.
set /p "ANS=Type ERASE to continue: "
if /I not "%ANS%"=="ERASE" exit /b 0

pio pkg exec --package "tool-esptoolpy" -- esptool.py --chip esp32s3 --port "%PORT%" erase_flash
if errorlevel 1 (
  echo [ERROR] Erase failed.
  pause
  exit /b 1
)
echo [OK] Flash erased.
pause
