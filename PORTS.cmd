@echo off
setlocal
cd /d "%~dp0"
call scripts\common.cmd || (pause & exit /b %errorlevel%)
py -3 tools\find_ports.py
pause
