@echo off
setlocal
cd /d "%~dp0"
call scripts\common.cmd || (pause & exit /b %errorlevel%)
echo [YWD-RF] Cleaning build output...
pio run -t clean
exit /b %errorlevel%
