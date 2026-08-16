@echo off
rem Shared prerequisite check. Call from a root-level script after cd /d "%~dp0".

where py >nul 2>&1
if errorlevel 1 (
  echo [ERROR] Python launcher ^(py^) was not found.
  echo         Run SETUP.cmd after installing Python 3.11+.
  exit /b 20
)

py -3 -m platformio --version >nul 2>&1
if errorlevel 1 (
  echo [ERROR] PlatformIO Core is not installed for this Python.
  echo         Run SETUP.cmd first.
  exit /b 21
)

exit /b 0
