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
echo          YWD-RF PERIPHERAL PROBE
echo ==================================================
echo.
echo This image brings hardware up one stage at a time:
echo   1. Vext power
 echo   2. I2C pin levels and address scan
 echo   3. SSD1306/U8g2 initialization
 echo   4. SX1262 BUSY/DIO1 sanity checks
 echo   5. SPI + RadioLib initialization in standby only
 echo.
echo No RF transmission is performed by this probe.
echo.
echo [YWD-RF] Building peripheral probe and flashing %PORT%...
py -3 -m platformio run -e ywd_rf_probe -t upload --upload-port "%PORT%"
if errorlevel 1 (
  echo.
  echo [ERROR] Peripheral probe upload failed.
  pause
  exit /b 1
)

echo.
echo [OK] Probe firmware flashed.
echo The application COM port can change after reset.
timeout /t 2 /nobreak >nul
py -3 tools\find_ports.py

echo.
echo Open option 7 - Serial Monitor Only - at 115200 baud.
echo Copy the complete probe output back into ChatGPT.
echo.
pause
exit /b 0
