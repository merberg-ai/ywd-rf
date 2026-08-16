@echo off
setlocal
cd /d "%~dp0"
call scripts\common.cmd
if errorlevel 1 (pause & exit /b 20)
echo [YWD-RF] Building firmware...
py -3 -m platformio run
set "RC=%errorlevel%"
if not "%RC%"=="0" echo [ERROR] Build failed with code %RC%.
if "%RC%"=="0" echo [OK] Build complete.
exit /b %RC%
