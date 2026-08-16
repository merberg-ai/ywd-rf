@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\common.cmd
if errorlevel 1 (pause & exit /b 20)

for /f "usebackq delims=" %%P in (`py -3 tools\select_port.py`) do set "PORT=%%P"
if not defined PORT (
  echo [ERROR] No port selected.
  pause
  exit /b 2
)

call FLASH.cmd "%PORT%"
if errorlevel 1 exit /b 1
call MONITOR.cmd "%PORT%"
