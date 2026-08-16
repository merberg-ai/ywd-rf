@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\common.cmd || (pause & exit /b %errorlevel%)

if not "%~1"=="" (
  set "PORT=%~1"
) else (
  for /f "usebackq delims=" %%P in (`py -3 tools\select_port.py`) do set "PORT=%%P"
)

if not defined PORT (
  echo [ERROR] No COM port selected.
  pause
  exit /b 2
)

echo [YWD-RF] Building and flashing %PORT%...
pio run -t upload --upload-port "%PORT%"
if errorlevel 1 (
  echo.
  echo Upload failed. If the ESP32-S3 is stuck, try recovery mode:
  echo   1. Hold BOOT/PRG
  echo   2. Tap and release RESET
  echo   3. Release BOOT/PRG
  echo   4. Run FLASH.cmd again
  pause
  exit /b 1
)

echo [OK] Flash complete: %PORT%
exit /b 0
