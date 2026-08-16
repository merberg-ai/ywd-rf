@echo off
setlocal
cd /d "%~dp0"
call scripts\common.cmd || (pause & exit /b %errorlevel%)
echo [YWD-RF] Building firmware...
pio run
set "RC=%errorlevel%"
if not "%RC%"=="0" echo [ERROR] Build failed with code %RC%.
if "%RC%"=="0" echo [OK] Build complete.
exit /b %RC%
