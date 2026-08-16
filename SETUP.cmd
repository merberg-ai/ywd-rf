@echo off
setlocal EnableExtensions
cd /d "%~dp0"

echo ==================================================
echo              YWD-RF WINDOWS SETUP
echo ==================================================
echo.

where py >nul 2>&1
if errorlevel 1 (
  echo [ERROR] Python launcher ^(py^) was not found.
  echo Install Python 3.11+ from python.org with the Python launcher enabled,
  echo then run this script again.
  pause
  exit /b 1
)

for /f "tokens=*" %%V in ('py -3 --version 2^>^&1') do echo [OK] %%V

echo.
echo Checking PlatformIO Core...
py -3 -m platformio --version >nul 2>&1
if errorlevel 1 (
  echo PlatformIO not found. Installing/upgrading in your user account...
  py -3 -m pip install --user --upgrade platformio
  if errorlevel 1 goto :fail
)

for /f "tokens=*" %%V in ('py -3 -m platformio --version') do echo [OK] %%V

echo.
echo Installing project platform/libraries and performing first build...
py -3 -m platformio run
if errorlevel 1 goto :fail

echo.
echo Connected serial devices:
py -3 tools\find_ports.py

echo.
echo ==================================================
echo [OK] YWD-RF development environment is ready.
echo Run YWD-RF.cmd for the normal workflow.
echo ==================================================
pause
exit /b 0

:fail
echo.
echo [ERROR] Setup failed. Review the output above.
pause
exit /b 1
