@echo off
setlocal
echo %~1 | findstr /I "username" >nul
if %errorlevel% equ 0 (
    echo oauth2
) else (
    echo %GITEE_TOKEN%
)
