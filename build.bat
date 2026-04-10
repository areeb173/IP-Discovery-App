@echo off
REM Build script for IP Finder Installer
REM Requires: Python, Node.js, PyInstaller, and InnoSetup

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

REM Check Node/npm
npm --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: npm not found. Install Node.js from https://nodejs.org
    pause
    exit /b 1
)
echo [OK] npm found

REM Check/Install PyInstaller
python -m pip show pyinstaller >nul 2>&1
if %errorlevel% neq 0 (
    echo Installing PyInstaller...
    python -m pip install pyinstaller
)
echo [OK] PyInstaller ready

REM Install Flask deps
echo Installing Flask dependencies...
python -m pip install flask flask-cors >nul 2>&1
echo [OK] Flask ready

REM Check InnoSetup
where iscc.exe >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files (x86)\Inno Setup 6\iscc.exe" (
        set "PATH=%PATH%;C:\Program Files (x86)\Inno Setup 6"
        echo [OK] InnoSetup found
    ) else if exist "C:\Program Files\Inno Setup 6\iscc.exe" (
        set "PATH=%PATH%;C:\Program Files\Inno Setup 6"
        echo [OK] InnoSetup found
    ) else (
        echo ERROR: InnoSetup not found
        echo Download from: https://jrsoftware.org/isdl.php
        pause
        exit /b 1
    )
) else (
    echo [OK] InnoSetup found
)

REM Clean old builds
echo.
echo Cleaning old builds...
if exist "dist" rmdir /s /q dist >nul 2>&1
if exist "build" rmdir /s /q build >nul 2>&1

REM Step 1: Build React frontend
echo.
echo [1/3] Building React frontend...
cd frontend
call npm install
if %errorlevel% neq 0 (
    echo ERROR: npm install failed
    cd ..
    pause
    exit /b 1
)
call npm run build
if %errorlevel% neq 0 (
    echo ERROR: npm run build failed
    cd ..
    pause
    exit /b 1
)
cd ..
echo [OK] Frontend built

REM Step 2: Build backend executable
echo.
echo [2/3] Building backend executable...
python -m PyInstaller IPFinder.spec
if %errorlevel% neq 0 (
    echo ERROR: PyInstaller build failed
    pause
    exit /b 1
)
echo [OK] Backend executable built

REM Step 3: Create installer
echo.
echo [3/3] Creating installer...
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
