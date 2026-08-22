@echo off
setlocal EnableExtensions
cd /d "%~dp0"

:menu
cls
echo ==================================================
echo               YWD-RF DEVELOPMENT
echo ==================================================
echo.
echo   1. Setup / Repair Environment
echo   2. Detect Connected Boards
echo   3. Build Firmware
echo   4. Flash One Board
echo   5. Flash Both Boards
echo   6. Flash + Serial Monitor
echo   7. Serial Monitor Only
echo   8. Clean Build
echo   9. Recovery / Erase Flash
echo  10. Flash Minimal Diagnostic Firmware
echo.
echo   Q. Quit
echo.
set /p "CHOICE=Select: "

if "%CHOICE%"=="1" call SETUP.cmd
if "%CHOICE%"=="2" call PORTS.cmd
if "%CHOICE%"=="3" call BUILD.cmd
if "%CHOICE%"=="4" call FLASH.cmd
if "%CHOICE%"=="5" call FLASH-BOTH.cmd
if "%CHOICE%"=="6" call DEV.cmd
if "%CHOICE%"=="7" call MONITOR.cmd
if "%CHOICE%"=="8" call CLEAN.cmd
if "%CHOICE%"=="9" call ERASE.cmd
if "%CHOICE%"=="10" call DIAGNOSTIC.cmd
if /I "%CHOICE%"=="Q" exit /b 0

goto :menu
