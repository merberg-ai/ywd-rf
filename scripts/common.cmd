@echo off
setlocal

rem Shared helper. Call with: call scripts\common.cmd

set "YWD_ROOT=%~dp0.."
for %%I in ("%YWD_ROOT%") do set "YWD_ROOT=%%~fI"
pushd "%YWD_ROOT%" >nul

where pio >nul 2>&1
if errorlevel 1 (
  echo [ERROR] PlatformIO Core ^(pio^) was not found in PATH.
  echo         Run SETUP.cmd first.
  popd >nul
  exit /b 20
)

popd >nul
exit /b 0
