@echo off
setlocal
cd /d "%~dp0"
call scripts\common.cmd
if errorlevel 1 (pause & exit /b 20)
echo [YWD-RF] Cleaning build output...
py -3 -m platformio run -t clean
exit /b %errorlevel%
