; QueMusic Inno Setup 脚本
;
; 基于原有 QueMusicPack.iss 整理，保留原 AppId（升级路径不变），改为相对路径，
; 并去掉应用并不支持的 .myp 文件关联。
;
; 用法：
;   iscc /DBuildDir="C:\path\to\deployed\bin" packaging\windows\QueMusic.iss
;
; BuildDir 应是已执行 windeployqt 的目录（包含 QueMusic.exe 与 Qt 运行库）。
;
; 签名：先签 build\bin 里的 exe/dll，再编译安装包，最后签安装包。
;   iscc /DBuildDir="...\bin" /DMyAppVersion=0.6.0 packaging\windows\QueMusic.iss
;   packaging\windows\sign.ps1 -Path ..\..\dist\QueMusic-0.6.0-win64-setup.exe -Thumbprint ...
;
; 若在 Inno IDE / 命令行配置了 SignTool，可启用下面两行让卸载器也带签名：
; SignTool=signtool
; SignedUninstaller=yes

#define MyAppName "QueMusic"
#ifndef MyAppVersion
  #define MyAppVersion "0.6.0"
#endif
#define MyAppPublisher "BroNekoX"
#define MyAppURL "https://quemusic.top"
#define MyAppExeName "QueMusic.exe"

#ifndef BuildDir
  #define BuildDir "..\..\build\Desktop_Qt_6_10_3_llvm_mingw_64_bit_Release\bin"
#endif

[Setup]
; 保持原安装器的 AppId，已装用户可原地升级
AppId={{EE637251-3796-4B03-A25B-2060B00E221E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
InfoBeforeFile=..\..\NOTICE
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=..\..\dist
OutputBaseFilename=QueMusic-{#MyAppVersion}-win64-setup
SetupIconFile=..\..\resources\icon.ico
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\Chinese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
