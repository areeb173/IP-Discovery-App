#!/usr/bin/env python3
"""
Build script for IP Finder
Steps:
  1. Build React frontend (npm run build)
  2. Build Flask backend exe (PyInstaller)
  3. Build Electron GUI (electron-builder)
  4. Package everything into MSI installer (WiX v3)
"""

import os
import sys
import subprocess
import shutil

WIX_BIN = r"C:\Program Files (x86)\WiX Toolset v3.14\bin"

def run_command(cmd, description, cwd=None):
    print(f"\n{description}...")
    try:
        subprocess.run(cmd, shell=True, check=True, cwd=cwd)
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: {description} failed with code {e.returncode}")
        return False

def check_tool(tool_name, download_url=None):
    result = subprocess.run(f"where {tool_name}", shell=True, capture_output=True)
    if result.returncode != 0:
        if tool_name == "candle.exe" or tool_name == "light.exe":
            if os.path.exists(os.path.join(WIX_BIN, tool_name)):
                os.environ["PATH"] += f";{WIX_BIN}"
                print(f"[OK] {tool_name} found at {WIX_BIN}")
                return True
        print(f"ERROR: {tool_name} not found")
        if download_url:
            print(f"  Download: {download_url}")
        return False
    print(f"[OK] {tool_name} found")
    return True

def main():
    print("\n" + "="*50)
    print("IP Finder Full Build")
    print("="*50 + "\n")

    root = os.path.dirname(os.path.abspath(__file__))
    frontend_dir = os.path.join(root, "frontend")
    electron_dir = os.path.join(root, "electron")
    build_dir = os.path.join(root, "build")
    electron_unpacked = os.path.join(build_dir, "electron", "win-unpacked")

    # Prerequisites
    print("Checking prerequisites...")
    if sys.version_info < (3, 7):
        print("ERROR: Python 3.7+ required")
        return False
    if not check_tool("npm"):
        return False
    if not check_tool("candle.exe", "https://github.com/wixtoolset/wix3/releases"):
        return False
    if not check_tool("light.exe", "https://github.com/wixtoolset/wix3/releases"):
        return False

    try:
        import PyInstaller
    except ImportError:
        subprocess.run([sys.executable, "-m", "pip", "install", "pyinstaller"], check=True)

    subprocess.run([sys.executable, "-m", "pip", "install", "flask", "flask-cors", "fpdf2"], check=True)

    # Check OllamaSetup.exe exists
    ollama_setup = os.path.join(root, "OllamaSetup.exe")
    if not os.path.exists(ollama_setup):
        print("ERROR: OllamaSetup.exe not found in project root.")
        print("  Download from: https://ollama.com/download")
        return False

    # Clean
    print("\nCleaning old builds...")
    for folder in ["dist", os.path.join("build", "electron"), os.path.join("build", "msi")]:
        full = os.path.join(root, folder)
        if os.path.exists(full):
            shutil.rmtree(full)

    os.makedirs(os.path.join(build_dir, "msi"), exist_ok=True)

    # Step 1: React frontend
    print("\n[1/4] Building React frontend...")
    if not run_command("npm install", "npm install", cwd=frontend_dir):
        return False
    if not run_command("npm run build", "npm run build", cwd=frontend_dir):
        return False
    print("[OK] Frontend built")

    # Step 2: Flask backend exe
    print("\n[2/4] Building Flask backend with PyInstaller...")
    if not run_command(f"{sys.executable} -m PyInstaller IPFinder.spec", "PyInstaller", cwd=root):
        return False
    print("[OK] Backend built -> dist/IPFinder.exe")

    # Step 3: Electron app
    print("\n[3/4] Building Electron GUI...")
    if not run_command("npm install", "npm install (electron)", cwd=electron_dir):
        return False
    if not run_command("npm run build", "electron-builder", cwd=electron_dir):
        return False
    print("[OK] Electron app built")

    # Step 4: WiX MSI
    print("\n[4/4] Creating MSI with WiX v3...")

    # Harvest Electron files using heat.exe
    heat_cmd = (
        f'"{WIX_BIN}\\heat.exe" dir "{electron_unpacked}" '
        f'-o build\\msi\\ElectronComponents.wxs '
        f'-scom -frag -srd -sreg -gg -cg ElectronFiles '
        f'-dr INSTALLFOLDER -var var.ElectronSource'
    )
    if not run_command(heat_cmd, "Harvesting Electron files", cwd=root):
        return False

    # Compile installer.wxs
    candle_cmd = (
        f'"{WIX_BIN}\\candle.exe" '
        f'-arch x64 '
        f'-dElectronSource="{electron_unpacked}" '
        f'installer.wxs build\\msi\\ElectronComponents.wxs '
        f'-o build\\msi\\ '
        f'-ext "{WIX_BIN}\\WixUIExtension.dll" '
        f'-ext "{WIX_BIN}\\WixUtilExtension.dll"'
    )
    if not run_command(candle_cmd, "Compiling WiX sources", cwd=root):
        return False

    # Link into MSI
    light_cmd = (
        f'"{WIX_BIN}\\light.exe" '
        f'build\\msi\\installer.wixobj build\\msi\\ElectronComponents.wixobj '
        f'-o build\\IPFinder.msi '
        f'-ext "{WIX_BIN}\\WixUIExtension.dll" '
        f'-ext "{WIX_BIN}\\WixUtilExtension.dll" '
        f'-cultures:en-us'
    )
    if not run_command(light_cmd, "Linking MSI", cwd=root):
        return False

    print("\n" + "="*50)
    print("SUCCESS!")
    print("="*50)
    print("\nInstaller: build\\IPFinder.msi\n")
    return True

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)