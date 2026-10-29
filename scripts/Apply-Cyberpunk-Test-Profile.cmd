@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\cyberpunk-test-profile.ps1" -Mode Apply
set "result=%errorlevel%"
pause
exit /b %result%
