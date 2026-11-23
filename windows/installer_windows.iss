[Setup]
AppId={{B4E9A13C-7D22-4B8F-9A31-6C5E8F12D409}
AppName=Shanty Tail and Teeth EQ
AppVersion=1.0.0
AppPublisher=Adamsky Music
DefaultDirName={commonpf64}\Common Files\VST3
DisableDirPage=yes
DefaultGroupName=Shanty Tail and Teeth EQ
OutputDir=..\dist
OutputBaseFilename=ShantyTailAndTeethEQ-1.0.0-Windows-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (VST3 + Standalone)"
Name: "vst3only"; Description: "VST3 Plugin only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Shanty Tail and Teeth EQ VST3 Plugin (64-bit)"; Types: full vst3only custom; Flags: fixed
Name: "standalone"; Description: "Shanty Tail and Teeth EQ Standalone App"; Types: full custom

[Files]
Source: "..\build-win\ShantyTailAndTeethEQ_artefacts\Release\VST3\Shanty Tail and Teeth EQ.vst3\*"; DestDir: "{commoncf64}\VST3\Shanty Tail and Teeth EQ.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build-win\ShantyTailAndTeethEQ_artefacts\Release\Standalone\Shanty Tail and Teeth EQ.exe"; DestDir: "{commonpf64}\Adamsky Music\Shanty Tail and Teeth EQ"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\Shanty Tail and Teeth EQ"; Filename: "{commonpf64}\Adamsky Music\Shanty Tail and Teeth EQ\Shanty Tail and Teeth EQ.exe"; Components: standalone
Name: "{group}\Uninstall Shanty Tail and Teeth EQ"; Filename: "{uninstallexe}"
