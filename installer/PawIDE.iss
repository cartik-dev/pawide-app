; Inno Setup script for Paw IDE (Windows 10/11 x64).
; Build: iscc /DAppVersion=1.0.0 /DSourceDir=<dir with PawIDE.exe + Qt runtime> installer\PawIDE.iss
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\dist"
#endif

[Setup]
AppId={{6B1D2C58-7F0E-4C0B-9E55-3A8B4F9A1D77}
AppName=Paw IDE
AppVersion={#AppVersion}
AppVerName=Paw IDE {#AppVersion}
AppPublisher=Paw IDE
AppPublisherURL=https://github.com/cartik-dev/pawide-app
AppSupportURL=https://github.com/cartik-dev/pawide-app/issues
AppUpdatesURL=https://github.com/cartik-dev/pawide-app/releases
DefaultDirName={autopf}\Paw IDE
DefaultGroupName=Paw IDE
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\PawIDE.exe
SetupIconFile=..\assets\branding\paw-icon.ico
OutputDir=Output
OutputBaseFilename=PawIDE-Setup-x64
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
CloseApplications=yes
RestartApplications=no
; Re-running a newer installer updates in place. User data lives in %APPDATA% and is never touched.

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Paw IDE"; Filename: "{app}\PawIDE.exe"
Name: "{autodesktop}\Paw IDE"; Filename: "{app}\PawIDE.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\PawIDE.exe"; Description: "Launch Paw IDE"; Flags: nowait postinstall skipifsilent
