#!/usr/bin/env python3
"""
Build script for IP Finder Installer
Automates PyInstaller and InnoSetup build process
"""

import os
import sys
import subprocess
import shutil

def run_command(cmd, description):
    """Run a command and handle errors"""
    print(f"\n{description}...")
    try:
        result = subprocess.run(cmd, shell=True, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: {description} failed with code {e.returncode}")
        return False

def check_tool(tool_name, download_url=None):
    """Check if a tool is installed"""
    result = subprocess.run(f"where {tool_name}", shell=True, capture_output=True)
    if result.returncode != 0:
        # For InnoSetup, check common installation paths
        if tool_name == "iscc.exe":
            if os.path.exists(r"C:\Program Files (x86)\Inno Setup 6\iscc.exe"):
                # Add to PATH for this process
                os.environ["PATH"] += r";C:\Program Files (x86)\Inno Setup 6"
                print(f"[OK] {tool_name} found at default location")
                return True
            elif os.path.exists(r"C:\Program Files\Inno Setup 6\iscc.exe"):
                os.environ["PATH"] += r";C:\Program Files\Inno Setup 6"
                print(f"[OK] {tool_name} found at default location")
                return True
        
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
        print(f"ERROR: Python 3.7+ required")
        return False

    # Check/install PyInstaller
    try:
        import PyInstaller
    except ImportError:
        print("Installing PyInstaller...")
        subprocess.run([sys.executable, "-m", "pip", "install", "pyinstaller"], check=True)

    # Check InnoSetup
    if not check_tool("iscc.exe", "https://jrsoftware.org/isdl.php"):
        return False

    # Clean old build
    print("\nCleaning old builds...")
    if os.path.exists("build"):
        shutil.rmtree("build")
    if os.path.exists("dist"):
        shutil.rmtree("dist")

    # Build executable
    print("\n[1/2] Building executable with PyInstaller...")
    if not run_command(
        f'{sys.executable} -m PyInstaller --onefile --console --name IPFinder --add-data "keywords.txt:." local_dir_parser.py',
        "PyInstaller"
    ):
        return False

    # Create installer
    print("\n[2/2] Creating installer with InnoSetup...")
    if not run_command("iscc.exe /Q installer.iss", "InnoSetup"):
        return False

    print("\n" + "="*50)
    print("SUCCESS!")
    print("="*50)
    print("\nInstaller: build/IPFinder-Setup.exe")
    print()
    return True

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
