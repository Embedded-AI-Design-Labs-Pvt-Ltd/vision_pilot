@echo off
REM One-click Vision Pilot ADAS virtual-hardware demo (Windows)
setlocal
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_adas_demo.ps1" %*
exit /b %ERRORLEVEL%
