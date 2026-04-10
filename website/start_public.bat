@echo off
title ECU Dashboard — Public Access
color 0A

echo.
echo  =========================================================
echo   ECU Dashboard — Public Access Launcher
echo  =========================================================
echo.

REM ── Check which tunnel tool is available ──────────────────────
where cloudflared >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo  [OK] cloudflared found
    goto :use_cloudflare
)

where ngrok >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo  [OK] ngrok found
    goto :use_ngrok
)

echo  [!] No tunnel tool found. Installing cloudflared...
echo.
winget install Cloudflare.cloudflared
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo  [FAIL] winget install failed. Download manually:
    echo  https://github.com/cloudflare/cloudflared/releases/latest
    echo  Put cloudflared.exe in C:\Windows\System32\
    pause
    exit /b 1
)
goto :use_cloudflare

REM ── Start Flask server + Cloudflare tunnel ────────────────────
:use_cloudflare
echo  Starting Flask server...
start "ECU Flask Server" cmd /k "cd /d %~dp0 && python server.py"
timeout /t 3 /nobreak >nul

echo.
echo  Starting Cloudflare Tunnel...
echo  Your public URL will appear below (look for trycloudflare.com):
echo  ─────────────────────────────────────────────────────────────
cloudflared tunnel --url http://localhost:5000
goto :end

REM ── Start Flask server + ngrok tunnel ────────────────────────
:use_ngrok
echo  Starting Flask server...
start "ECU Flask Server" cmd /k "cd /d %~dp0 && python server.py"
timeout /t 3 /nobreak >nul

echo.
echo  Starting ngrok tunnel...
echo  Your public URL will appear in the ngrok window:
echo  ─────────────────────────────────────────────────────────────
ngrok http 5000

:end
pause
