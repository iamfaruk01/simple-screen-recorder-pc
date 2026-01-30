; ===========================================
; Simple Screen Recorder - Inno Setup Script
; ===========================================
; This script creates a professional Windows installer
; with Next → Next → Install wizard

#define MyAppName "Simple Screen Recorder"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Faruk Khan"
#define MyAppURL "https://github.com/iamfaruk01/simple-screen-recorder-pc"
#define MyAppExeName "SimpleScreenRecorder.exe"

[Setup]
; Application info
AppId={{8A7B4D2E-5C6F-4A8B-9D0E-1F2A3B4C5D6E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Installation settings
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; Output settings - where the setup.exe will be created
OutputDir=..\dist\installer
OutputBaseFilename=SimpleScreenRecorder-Setup-{#MyAppVersion}

; Compression
Compression=lzma2/ultra64
SolidCompression=yes

; Windows version requirements
MinVersion=10.0

; Visual settings
WizardStyle=modern
SetupIconFile=..\assets\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}

; Privileges (doesn't require admin for per-user install)
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; License file (optional)
; LicenseFile=..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce

[Files]
; Main application
Source: "..\dist\SimpleScreenRecorder\SimpleScreenRecorder.exe"; DestDir: "{app}"; Flags: ignoreversion

; FFmpeg (required for video encoding)  
Source: "..\dist\SimpleScreenRecorder\ffmpeg.exe"; DestDir: "{app}"; Flags: ignoreversion

; README
Source: "..\dist\SimpleScreenRecorder\README.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Start Menu shortcut
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

; Desktop shortcut
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; Option to launch app after installation
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Clean up any files created during runtime
Type: filesandordirs; Name: "{app}\recordings"
