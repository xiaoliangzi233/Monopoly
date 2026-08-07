@echo off
setlocal
set "ROOT=%~dp0"
cd /d "%ROOT%"

echo ============================================
echo   Shengshi Baiye - One Click Release
echo ============================================

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\publish_release.ps1" -Yes %*
set "RESULT=%ERRORLEVEL%"

echo.
if "%RESULT%"=="0" (
    echo Release request completed successfully.
) else (
    echo [ERROR] Release failed with exit code %RESULT%.
)
echo.
pause
exit /b %RESULT%
