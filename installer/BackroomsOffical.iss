#define MyAppName "Backrooms Offical"
#define MyAppVersion GetEnv("BACKROOMS_VERSION")
#define MyAppExeName "Backrooms Offical.exe"

[Setup]
AppId={{E7D899A8-53A1-4A29-A760-5CCEA3493C8C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=spongebobtdgameplay-prog
DefaultDirName={localappdata}\Backrooms Offical
DefaultGroupName=Backrooms Offical
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=Backrooms-Offical-Windows-x64
SetupIconFile=..\assets\icon\Backrooms.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
RestartApplications=no

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "..\build\Release\Backrooms Offical.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\SDL3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Backrooms Offical"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\Backrooms Offical"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch Backrooms Offical"; Flags: nowait postinstall skipifsilent
