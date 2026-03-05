@echo off
REM Build script for IP Finder Installer
REM Requires: Python, PyInstaller, and InnoSetup

echo.
echo ===============================================
echo IP Finder Installer Build
echo ===============================================
echo.

REM Check Python
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Python not found
    pause
    exit /b 1
)
echo [OK] Python found

REM Check/Install PyInstaller
python -m pip show pyinstaller >nul 2>&1
if %errorlevel% neq 0 (
    echo Installing PyInstaller...
    python -m pip install pyinstaller
)
echo [OK] PyInstaller ready

REM Check InnoSetup
where iscc.exe >nul 2>&1
if %errorlevel% neq 0 (
    REM Try common installation path
    if exist "C:\Program Files (x86)\Inno Setup 6\iscc.exe" (
        set "PATH=%PATH%;C:\Program Files (x86)\Inno Setup 6"
        echo [OK] InnoSetup found at default location
    ) else if exist "C:\Program Files\Inno Setup 6\iscc.exe" (
        set "PATH=%PATH%;C:\Program Files\Inno Setup 6"
        echo [OK] InnoSetup found at default location
    ) else (
        echo ERROR: InnoSetup not found
        echo Download from: https://jrsoftware.org/isdl.php
        pause
        exit /b 1
    )
) else (
    echo [OK] InnoSetup found
)

echo.
echo [1/2] Building executable...
if exist "dist" rmdir /s /q dist >nul 2>&1
if exist "build" rmdir /s /q build >nul 2>&1
pyinstaller --onefile --console --name IPFinder --add-data "keywords.txt:." local_dir_parser.py
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)
echo [OK] Executable built

echo.
echo [2/2] Creating installer...
iscc.exe /Q installer.iss
if %errorlevel% neq 0 (
    echo ERROR: Installer creation failed
    pause
    exit /b 1
)
echo [OK] Installer created

echo.
echo ===============================================
echo SUCCESS!
echo ===============================================
echo.
echo Your installer: build\IPFinder-Setup.exe
echo.
pause
