#!/usr/bin/env python3
"""
Build script for IP Finder Installer
Automates frontend build, PyInstaller, and InnoSetup
"""

import os
import sys
import subprocess
import shutil

def run_command(cmd, description, cwd=None):
    """Run a command and handle errors"""
    print(f"\n{description}...")
    try:
        result = subprocess.run(cmd, shell=True, check=True, cwd=cwd)
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: {description} failed with code {e.returncode}")
        return False

def check_tool(tool_name, download_url=None):
    """Check if a tool is installed"""
    result = subprocess.run(f"where {tool_name}", shell=True, capture_output=True)
    if result.returncode != 0:
        if tool_name == "iscc.exe":
            for path in [r"C:\Program Files (x86)\Inno Setup 6", r"C:\Program Files\Inno Setup 6"]:
                if os.path.exists(os.path.join(path, "iscc.exe")):
                    os.environ["PATH"] += f";{path}"
                    print(f"[OK] {tool_name} found at {path}")
                    return True
        if tool_name == "npm":
            print(f"ERROR: npm not found. Install Node.js from https://nodejs.org")
            return False
        print(f"ERROR: {tool_name} not found")
        if download_url:
            print(f"  Download: {download_url}")
        return False
    print(f"[OK] {tool_name} found")
    return True

def main():
    print("\n" + "="*50)
    print("IP Finder Installer Build")
    print("="*50 + "\n")

    # Prerequisites
    print("Checking prerequisites...")

    if sys.version_info < (3, 7):
        print("ERROR: Python 3.7+ required")
        return False

    # Check npm
    if not check_tool("npm"):
        return False

    # Check/install PyInstaller
    try:
        import PyInstaller
    except ImportError:
        print("Installing PyInstaller...")
        subprocess.run([sys.executable, "-m", "pip", "install", "pyinstaller"], check=True)

    # Check/install Flask deps
    print("Checking Flask dependencies...")
    subprocess.run([sys.executable, "-m", "pip", "install", "flask", "flask-cors"], check=True)

    # Check InnoSetup
    if not check_tool("iscc.exe", "https://jrsoftware.org/isdl.php"):
        return False

    # Clean old builds
    print("\nCleaning old builds...")
    for folder in ["dist", os.path.join("build", "electron")]:
        if os.path.exists(folder):
            shutil.rmtree(folder)

    # Step 1: Build React frontend
    print("\n[1/4] Building React frontend...")
    frontend_dir = os.path.join(os.path.dirname(__file__), "frontend")
    electron_dir = os.path.join(os.path.dirname(__file__), "electron")
    if not os.path.exists(frontend_dir):
        print("ERROR: frontend/ directory not found")
        return False

    if not run_command("npm install", "Installing frontend dependencies", cwd=frontend_dir):
        return False
    if not run_command("npm run build", "Building React frontend", cwd=frontend_dir):
        return False

    frontend_dist = os.path.join(frontend_dir, "dist")
    if not os.path.exists(frontend_dist):
        print("ERROR: frontend/dist not found after build")
        return False
    print("[OK] Frontend built")

    # Step 2: Build backend executable with PyInstaller
    print("\n[2/4] Building backend executable with PyInstaller...")
    if not run_command(
        f'"{sys.executable}" -m PyInstaller IPFinder.spec',
        "PyInstaller"
    ):
        return False
    print("[OK] Backend executable built")

    # Step 3: Build Electron app 
    print("\n[3/4] Building Electron app...")
    if not run_command("npm install", "Installing Electron dependencies", cwd=electron_dir):
        return False    
    if not run_command("npm run build", "Building Electron app", cwd=electron_dir):
        return False
    print("[OK] Electron app built")

    # Step 4: Create installer with InnoSetup
    print("\n[4/4] Creating installer with InnoSetup...")
    if not run_command('"C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe" /Q installer.iss', "InnoSetup"):
        return False
    print("[OK] Installer created")

    print("\n" + "="*50)
    print("SUCCESS!")
    print("="*50)
    print("\nInstaller: build\\IPFinder-Setup.exe")
    return True

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
