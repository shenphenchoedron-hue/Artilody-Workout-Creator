; Inno Setup script for Artilody Workout Creator (Windows x64).
;
; Built in CI (.github/workflows/release.yml):
;   iscc /DAppVersion=1.0.0 /DSourceDir=<staging dir> /DOutputDir=<dist dir> packaging\windows\installer.iss
;
; SourceDir must contain the windeployqt'ed application:
;   "Artilody Workout Creator.exe", Qt DLLs, plugins, qml\ ...

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\dist\windows\Artilody Workout Creator"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

#define AppName      "Artilody Workout Creator"
#define AppExeName   "Artilody Workout Creator.exe"
#define AppPublisher "Artilody"
#define AppURL       "https://github.com/shenphenchoedron-hue/Artilody-Workout-Creator"

[Setup]
; Never change the AppId: it identifies the installation for upgrades/uninstall.
AppId={{08A70FEE-4592-42DD-ACF1-402DA7D440FB}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
#if FileExists(AddBackslash(SourcePath) + "..\..\LICENSE")
LicenseFile=..\..\LICENSE
#endif
OutputDir={#OutputDir}
OutputBaseFilename=ArtilodyWorkoutCreator-{#AppVersion}-windows-x64-setup
SetupIconFile=..\..\assets\icons\app-icon.ico
UninstallDisplayIcon={app}\{#AppExeName}
UninstallDisplayName={#AppName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog
MinVersion=10.0
VersionInfoVersion={#AppVersion}
VersionInfoProductName={#AppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"; IconFilename: "{app}\{#AppExeName}"
Name: "{autoprograms}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; IconFilename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
