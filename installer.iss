; IP Finder Installer Script for InnoSetup
; Download InnoSetup from: https://jrsoftware.org/isdl.php

[Setup]
AppName=IP Finder
AppVersion=1.0.0
AppPublisher=Your Company
AppPublisherURL=https://example.com
DefaultDirName={pf}\IPFinder
DefaultGroupName=IP Finder
OutputDir=build
OutputBaseFilename=IPFinder-Setup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64 arm64
ArchitecturesAllowed=x64 arm64 x86
MinVersion=0,6.1
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main executable
Source: "dist\IPFinder.exe"; DestDir: "{app}"; Flags: ignoreversion
; Keywords file
Source: "keywords.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\IP Finder"; Filename: "{app}\IPFinder.exe"
Name: "{group}\{cm:UninstallProgram,IP Finder}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\IP Finder"; Filename: "{app}\IPFinder.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\IPFinder.exe"; Description: "{cm:LaunchProgram,IP Finder}"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCU; Subkey: "Software\IPFinder"; Flags: uninsdeletekey
