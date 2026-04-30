; IP Finder Installer Script for InnoSetup
; Download InnoSetup from: https://jrsoftware.org/isdl.php

[Setup]
AppName=IP Finder
AppVersion=1.0.0
AppPublisher=The Fellows Consulting Group
AppPublisherURL=https://example.com
DefaultDirName={pf}\IPFinder
DefaultGroupName=IP Finder
OutputDir=build
OutputBaseFilename=IPFinder-Setup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64 
ArchitecturesAllowed=x64 
MinVersion=0,6.1
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "build\electron\win-unpacked\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "OllamaSetup.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\IP Finder"; Filename: "{app}\IP Finder.exe"
Name: "{group}\{cm:UninstallProgram,IP Finder}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\IP Finder"; Filename: "{app}\IP Finder.exe"; Tasks: desktopicon

[Run]
Filename: "{tmp}\OllamaSetup.exe"; Parameters: "/S"; StatusMsg: "Installing Ollama (AI engine)..."; Flags: waituntilterminated; Check: not FileExists(ExpandConstant('{pf}\Ollama\ollama.exe'))
Filename: "{app}\IP Finder.exe"; Description: "{cm:LaunchProgram,IP Finder}"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCU; Subkey: "Software\IPFinder"; Flags: uninsdeletekey

