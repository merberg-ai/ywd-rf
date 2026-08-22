@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\common.cmd
if errorlevel 1 (pause & exit /b 20)

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

echo ==================================================
echo        YWD-RF MINIMAL DIAGNOSTIC FIRMWARE
echo ==================================================
echo.
echo This image deliberately DOES NOT initialize:
echo   - Vext / OLED
 echo   - LoRa SPI
 echo   - SX1262 radio
 echo.
echo It only starts the ESP32 runtime, both debug serial paths,
echo and the onboard status LED heartbeat.
echo.
echo [YWD-RF] Building diagnostic image and flashing %PORT%...
py -3 -m platformio run -e ywd_rf_diag -t upload --upload-port "%PORT%"
if errorlevel 1 (
  echo.
  echo [ERROR] Diagnostic upload failed.
  echo Try recovery mode: hold BOOT/PRG, tap RESET, release RESET,
  echo then release BOOT/PRG and run DIAGNOSTIC.cmd again.
  pause
  exit /b 1
)

echo.
echo [OK] Diagnostic firmware flashed.
echo.
echo Expected behavior after reset:
echo   1. Onboard LED toggles about every 500 ms.
echo   2. Debug text repeats every 2 seconds.
echo   3. Logs are sent to BOTH native USB CDC and UART0 at 115200.
echo.
echo The application COM port may differ from the bootloader/upload port.
echo Connected serial devices after flash:
timeout /t 2 /nobreak >nul
py -3 tools\find_ports.py

echo.
echo Run MONITOR.cmd and select the application port that appears.
echo If there are two plausible ports, try both; one may be USB CDC and
 echo the other a USB-to-UART bridge.
echo.
pause
exit /b 0
