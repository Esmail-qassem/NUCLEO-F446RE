@echo off
setlocal enabledelayedexpansion

:: Set toolchain path (properly quoted)
set "TOOLCHAIN_PATH=C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin"
set "PATH=%PATH%;%TOOLCHAIN_PATH%"

:: Set Git Bash path
set "GIT_BASH=C:\Program Files\Git\bin\bash.exe"

:: Default command if no arguments
if "%~1"=="" (
    "%GIT_BASH%" -c "make all"
    goto :end
)

:: Process commands
if /i "%~1"=="-j" (
    "%GIT_BASH%" -c "make -j all"
) else if /i "%~1"=="clean" (
    "%GIT_BASH%" -c "make clean"
) else if /i "%~1"=="flash" (
    "%GIT_BASH%" -c "make flash"
) else if /i "%~1"=="erase" (
    "%GIT_BASH%" -c "make erase"
) else if /i "%~1"=="-a" (
    "%GIT_BASH%" -c "APPLICATION_ONLY=1 make clean && make all"
) else (
    echo Unknown command: %~1
    echo Available commands:
    echo   -j      : Build with parallel jobs
    echo   clean   : Clean build artifacts
    echo   flash   : Flash the device
    echo   erase   : Erase the device
    echo   -a      : Build with APPLICATION_ONLY flag
)

:end
endlocal