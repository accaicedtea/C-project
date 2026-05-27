#define MyAppName "GtkApp"
#define MyAppVersion "1.0"
#define MyAppPublisher "Developer"
#define MyAppURL "https://github.com/tuouser/gtkapp"
#define MyAppExeName "gtkapp.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=Output
OutputBaseFilename=gtkapp-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible arm64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"

[Files]
; --- File per x64 ---
Source: "dist\win-x64\gtkapp.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: IsX64
Source: "dist\win-x64\*.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: IsX64
Source: "dist\win-x64\lib\*"; DestDir: "{app}\lib"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsX64
Source: "dist\win-x64\share\*"; DestDir: "{app}\share"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsX64

; --- File per ARM64 ---
Source: "dist\win-arm64\gtkapp.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: IsARM64
Source: "dist\win-arm64\*.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: IsARM64
Source: "dist\win-arm64\lib\*"; DestDir: "{app}\lib"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsARM64
Source: "dist\win-arm64\share\*"; DestDir: "{app}\share"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsARM64

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: unchecked

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
function IsX64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = paX64);
end;

function IsARM64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = paARM64);
end;
