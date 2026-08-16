@echo off
setlocal
cd /d "%~dp0"
call scripts\common.cmd
if errorlevel 1 (pause & exit /b 20)
py -3 tools\find_ports.py
pause
