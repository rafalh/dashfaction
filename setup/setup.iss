; Preprocessor variables
#define SrcRootDir ".."
#define BinDir "..\build\Release\bin"
#define PatchesDir "patches\output"
#define AppVer "1.5.1-dev"

[Setup]
AppId={{BDD60DE7-9374-463C-8E74-8227EB03E28F}
AppName=Dash Faction
AppVersion={#AppVer}
AppPublisher=rafalh
AppPublisherURL=https://ravin.tk/
AppSupportURL=https://ravin.tk/
AppUpdatesURL=https://ravin.tk/
DefaultDirName={pf}\Dash Faction
DefaultGroupName=Dash Faction
InfoBeforeFile={#SrcRootDir}\README.md
OutputBaseFilename=DashFaction-{#AppVer}-setup
Compression=lzma2/max
SolidCompression=yes
OutputDir=build
SetupLogging=yes
ChangesAssociations=yes

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "rfproto"; Description: "Register rf:// protocol handler"; GroupDescription: "Other options:"
Name: "rflassoc"; Description: "Associate .rfl file extension with Dash Faction Level Editor"; GroupDescription: "Other options:"
Name: "fftracker"; Description: "Set rfgt.factionfiles.com as multiplayer tracker"; GroupDescription: "Other options:"
Name: "patchgame"; Description: "Install needed game patches"; GroupDescription: "Other options:"; Check: "PatchGameTaskCheck"

[Files]
Source: "{#BinDir}\DashFactionLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BinDir}\CrashHandler.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BinDir}\DashEditor.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BinDir}\DashFaction.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRootDir}\dashfaction.vpp"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRootDir}\licensing-info.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRootDir}\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRootDir}\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
; RTPatch patches (extracted from official 1.20 patches)
Source: "{#PatchesDir}\patchw32.dll"; Flags: dontcopy
Source: "{#PatchesDir}\rf120_na.rtp"; Flags: dontcopy
Source: "{#PatchesDir}\rf120_gr.rtp"; Flags: dontcopy
Source: "{#PatchesDir}\rf120_fr.rtp"; Flags: dontcopy
; minibsdiff patches (converts existing files to ones compatible with 1.20 version)
Source: "{#BinDir}\minibsdiff.exe"; Flags: dontcopy
Source: "{#PatchesDir}\RF-1.20-gr.exe.mbsdiff"; Flags: dontcopy
Source: "{#PatchesDir}\RF-1.20-fr.exe.mbsdiff"; Flags: dontcopy
Source: "{#PatchesDir}\RF-1.21.exe.mbsdiff"; Flags: dontcopy
Source: "{#PatchesDir}\RF-1.21-steam.exe.mbsdiff"; Flags: dontcopy
Source: "{#PatchesDir}\tables-gog-gr.vpp.mbsdiff"; Flags: dontcopy

; Allow write by non-admin users in directories used by the game (may be needed when elevation is not used)
[Dirs]
Name: "{code:GetGameDir}"; Permissions: users-modify
Name: "{code:GetGameDir}\user_maps\multi"; Permissions: users-modify
Name: "{code:GetGameDir}\screenshots"; Permissions: users-modify
Name: "{code:GetGameDir}\logs"; Permissions: users-modify

[Icons]
Name: "{group}\Dash Faction"; Filename: "{app}\DashFactionLauncher.exe"
Name: "{commondesktop}\Dash Faction"; Filename: "{app}\DashFactionLauncher.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\DashFactionLauncher.exe"; Description: "{cm:LaunchProgram,Dash Faction}"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCU; Subkey: "Software\Volition\Red Faction\Dash Faction"; ValueType: "string"; ValueName: "Executable Path"; ValueData: "{code:GetFinalGameExePath}"
Root: HKCU; Subkey: "Software\Volition\Red Faction"; ValueType: "string"; ValueName: "GameTracker"; ValueData: "rfgt.factionfiles.com"; Tasks: fftracker
; rf:// protocol
Root: HKCR; Subkey: "rf"; ValueType: "string"; ValueData: "URL:Red Faction Protocol"; Flags: uninsdeletekey; Tasks: rfproto
Root: HKCR; Subkey: "rf"; ValueType: "string"; ValueName: "URL Protocol"; ValueData: ""; Tasks: rfproto
Root: HKCR; Subkey: "rf\DefaultIcon"; ValueType: "string"; ValueData: "{app}\DashFactionLauncher.exe,0"; Tasks: rfproto
Root: HKCR; Subkey: "rf\shell\open\command"; ValueType: "string"; ValueData: """{app}\DashFactionLauncher.exe"" -game -url %1"; Tasks: rfproto
; rfl file extension association
Root: HKCR; Subkey: ".rfl"; ValueType: "string"; ValueData: "DashFactionLevelEditor"; Flags: uninsdeletekey; Tasks: rflassoc
Root: HKCR; Subkey: "DashFactionLevelEditor"; ValueType: "string"; ValueData: "Dash Faction Level Editor"; Flags: uninsdeletekey; Tasks: rflassoc
;Root: HKCR; Subkey: "DashFactionLevelEditor\DefaultIcon"; ValueType: "string"; ValueData: "{app}\DashFactionLauncher.exe,0"; Tasks: rflassoc
Root: HKCR; Subkey: "DashFactionLevelEditor\shell\open\command"; ValueType: "string"; ValueData: """{app}\DashFactionLauncher.exe"" -editor -level %1"; Tasks: rflassoc


[CustomMessages]
RFExeLocation=Please specify location of RF.exe file from Red Faction installation directory:
GameNeedsPatches=Detected Red Faction version is not directly supported by Dash Faction. Setup program will install all the needed patches automatically.%n%nPatches that will be installed:%n
UnkGameExeVersion=Unknown RF.exe version (SHA1 = %1). Dash Faction will not function correctly.%nFind help at http://redfaction.help and http://redfaction.chat (Discord).%n%nDo you want to ignore this error and continue?
UnkTablesVppVersion=Unknown tables.vpp version (SHA1 = %1). Multiplayer will not function correctly.%nFind help at http://redfaction.help and http://redfaction.chat (Discord).%n%nDo you want to ignore this error and continue?

[Code]
type
    TPatchType = (RTPatch, BSDiff);
    TPatchInfo = record
        DisplayName: String;
        FileName: String;
        PatchType: TPatchType;
        SrcFile: String;
        DestFile: String;
    end;
    TPatchArray = array [0..7] of TPatchInfo;

var
    SelectGameExePage: TInputFileWizardPage;
    Patches: array of TPatchInfo;
    PatchesDisplayNames: TStringList;
    GameExeFileNameAfterPatches: String;

function GetUserProvidedGameExePath(Param: String): String;
begin
    Result := SelectGameExePage.Values[0];
end;

function IsPatchGameTaskSelected(): Boolean;
begin
    Result := Pos('patchgame', WizardSelectedTasks(false)) > 0;
end;

function GetFinalGameExePath(Param: String): String;
begin
    if IsPatchGameTaskSelected() and (GameExeFileNameAfterPatches <> '') then
        Result := ExtractFileDir(SelectGameExePage.Values[0]) + '\' + GameExeFileNameAfterPatches
    else
        Result := SelectGameExePage.Values[0];
end;

function GetGameDir(Param: String): String;
begin
    Result := ExtractFileDir(SelectGameExePage.Values[0]);
end;

procedure AddBSDiffPatch(DisplayName: String; FileName: String; SrcFile: String; DestFile: String);
begin
    SetLength(Patches, Length(Patches) + 1);
    Patches[Length(Patches) - 1].DisplayName := DisplayName;
    Patches[Length(Patches) - 1].FileName := FileName;
    Patches[Length(Patches) - 1].PatchType := BSDiff;
    Patches[Length(Patches) - 1].SrcFile := SrcFile;
    Patches[Length(Patches) - 1].DestFile := DestFile;
    PatchesDisplayNames.Add(DisplayName);
end;

procedure AddGameExeBSDiffPatch(DisplayName: String; PatchFileName: String);
begin
    AddBSDiffPatch(DisplayName, PatchFileName, ExtractFileName(SelectGameExePage.Values[0]), 'RF_120na.exe');
    GameExeFileNameAfterPatches := 'RF_120na.exe';
end;

procedure AddRTPatch(DisplayName: String; FileName: String);
begin
    SetLength(Patches, Length(Patches) + 1);
    Patches[Length(Patches) - 1].DisplayName := DisplayName;
    Patches[Length(Patches) - 1].FileName := FileName;
    Patches[Length(Patches) - 1].PatchType := RTPatch;
    PatchesDisplayNames.Add(DisplayName);
end;

procedure ResetPatchList();
begin
    SetLength(Patches, 0);
    GameExeFileNameAfterPatches := '';
    PatchesDisplayNames := TStringList.Create;
end;

function DetermineNeededPatches(GameExePath: String): Boolean;
var
    GameExeSHA1: String;
    TablesVppSHA1: String;
begin
    ResetPatchList;
    Result := True;
    // RF.exe patching
    GameExeSHA1 := GetSHA1OfFile(GameExePath);
    case GameExeSHA1 of
        'f94f2e3f565d18f75ab6066e77f73a62a593fe03':
            // 1.20 NA - this is directly supported version
            begin end;
        '4140f7619b6427c170542c66178b47bd48795a99':
            begin end;
        'ddbb3e4fa89301eca9f0f547c93a18c18b4bc2ff':
            AddGameExeBSDiffPatch('1.20 GR -> 1.20 NA (RF.exe patch)', 'RF-1.20-gr.exe.mbsdiff');
        '911e3c2d767e56b7b2d5e07ffe07372c82b9dd9d':
            AddGameExeBSDiffPatch('1.20 FR -> 1.20 NA (RF.exe patch)', 'RF-1.20-fr.exe.mbsdiff');
        'abc97f3599caf50f540865a80e34c3feb76767a0':
            AddGameExeBSDiffPatch('1.21 with Steam Wrapper -> 1.20 NA (RF.exe patch)', 'RF-1.21-steam.exe.mbsdiff');
        'd3e42d1d24ad730b28d3e562042bd2c8cf2ab64f':
            AddGameExeBSDiffPatch('1.21 -> 1.20 NA (RF.exe patch)', 'RF-1.21.exe.mbsdiff');
        '23b93a5e72c9f31b3d365ce5feffc9f35ae1ab96':
            AddRTPatch('1.10 NA -> 1.20 NA (official patch)', 'rf120_na.rtp');
        // TODO: add 1.10 GR and 1.10 FR
        '640526799fed170ce110c71e1bcf0b23b063d1da':
            AddRTPatch('1.00 NA -> 1.20 NA (official patch)', 'rf120_na.rtp');
        '2c11d549787f6af655c37e8ef2c53293d02dbfbc':
        begin
            AddRTPatch('1.00 GR -> 1.20 GR (official patch)', 'rf120_gr.rtp');
            AddGameExeBSDiffPatch('1.20 GR -> 1.20 NA (RF.exe patch)', 'RF-1.20-gr.exe.mbsdiff');
        end;
        'fc6d877d5b423c004886e1f73bfbe72ad4d55e2d':
        begin
            AddRTPatch('1.00 FR -> 1.20 FR (official patch)', 'rf120_fr.rtp');
            AddGameExeBSDiffPatch('1.20 FR -> 1.20 NA (RF.exe patch)', 'RF-1.20-fr.exe.mbsdiff');
        end;
        else
        begin
            // Unknown version
            Log('Unknown RF.exe SHA1: ' + GameExeSHA1);
            if Result and (MsgBox(ExpandConstant('{cm:UnkGameExeVersion,' + GameExeSHA1 + '}'), mbError, MB_YESNO) = IDNO) then
                Result := False;
        end;
    end;
    // tables.vpp patching
    TablesVppSHA1 := GetSHA1OfFile(ExtractFileDir(GameExePath) + '\tables.vpp');
    case TablesVppSHA1 of
        'ded5e1b5932f47044ba760699d5931fda6bdc8ba':
            // 1.20 NA - this is directly supported version
            begin end;
        '672ba832f7133b93d576f84d971331597d93fce6':
            AddBSDiffPatch('1.21 GR GOG -> 1.20 NA (tables.vpp patch)', 'tables-gog-gr.vpp.mbsdiff', 'tables.vpp', 'tables.vpp');
        else
        begin
            // Unknown version
            Log('Unknown tables.vpp SHA1: ' + TablesVppSHA1);
            if Result and (MsgBox(ExpandConstant('{cm:UnkTablesVppVersion,' + TablesVppSHA1 + '}'), mbError, MB_YESNO) = IDNO) then
                Result := False;
        end;
    end;
    if not Result then
        ResetPatchList;
end;

function SelectGameExePageOnNextButtonClick(Sender: TWizardPage): Boolean;
begin
    if not FileExists(SelectGameExePage.Values[0]) then
    begin
        MsgBox('Specified file does not exist!', mbError, MB_OK);
        Result := False;
    end
    else
    begin
        Result := DetermineNeededPatches(SelectGameExePage.Values[0]);
        if Result and (Length(Patches) > 0) then
            Result := MsgBox(ExpandConstant('{cm:GameNeedsPatches}') + PatchesDisplayNames.Text, mbInformation, MB_OKCANCEL) = IDOK;
    end;
end;

function DetectGameExecutablePath(): String;
var
    NativeHKLM: Integer;
begin
    if IsWin64() then
        NativeHKLM := HKEY_LOCAL_MACHINE_64
    else
        NativeHKLM := HKEY_LOCAL_MACHINE;
    if RegQueryStringValue(HKEY_CURRENT_USER, 'SOFTWARE\Volition\Red Faction\Dash Faction', 'Executable Path', Result) then
        // Dash Faction options - nothing to do
    else if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Volition\Red Faction', 'InstallPath', Result) then
        // Install from CD
        Result := Result + '\RF.exe'
    else if RegQueryStringValue(NativeHKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 20530', 'InstallLocation', Result) then
        // Steam
        Result := Result + '\RF.exe'
    else if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\GOG.com\GOGREDFACTION', 'PATH', Result) then
        // GOG
        Result := Result + 'RF.exe'
    else
        // Fallback
        Result := 'C:\games\RedFaction\RF.exe';
end;

procedure CreateSelectGameExePage();
begin
    SelectGameExePage := CreateInputFilePage(wpSelectDir, 'Select RF.exe', 'Select Red Faction executable file', '');
    SelectGameExePage.Add(ExpandConstant('{cm:RFExeLocation}'), 'Executable files|*.exe|All files|*.*', '.exe');
    SelectGameExePage.Values[0] := DetectGameExecutablePath;
    SelectGameExePage.OnNextButtonClick := @SelectGameExePageOnNextButtonClick;
end;

function RTPatchApply32NoCall(CmdLine: PAnsiChar): Longint;
external 'RTPatchApply32NoCall@files:patchw32.dll cdecl delayload';

function ApplyRTPatch(WorkDir: String; PatchFile: String): Boolean;
var
    CmdLine: AnsiString;
    ResultCode: Longint;
begin
    ExtractTemporaryFile(PatchFile);
    CmdLine := '"' + WorkDir + '" "' + ExpandConstant('{tmp}\') + PatchFile + '"';
    Log('Running RTPatch with command line: ' + CmdLine);
    ResultCode := RTPatchApply32NoCall(CmdLine);
    Log('RTPatchApply32 result code: ' + IntToStr(ResultCode));
    Result := False;
    if ResultCode <> 0 then
        MsgBox('Failed to install patch ' + PatchFile + '. You have to patch the game manually!' + #13#13 +
            'Error details:' + #13 + 'RTPatchApply32NoCall returned error code: ' + IntToStr(ResultCode), mbError, MB_OK)
    else
        Result := True;
end;

function ApplyBSDiffPatch(WorkDir: String; PatchFile: String; SrcFile: String; DestFile: String): Boolean;
var
    Args: String;
    ResultCode: Integer;
begin
    ExtractTemporaryFile('minibsdiff.exe');
    ExtractTemporaryFile(PatchFile);
    Args := 'app "' + SrcFile + '" "' + ExpandConstant('{tmp}\') + PatchFile + '" "' + DestFile + '"';
    Log('Running minibsdiff ' + Args);
    Result := False;
    if not Exec(ExpandConstant('{tmp}\minibsdiff.exe'), Args, WorkDir, SW_SHOW, ewWaitUntilTerminated, ResultCode) then
        MsgBox('Failed to install patch ' + PatchFile + '. You have to patch the game manually!' + #13#13 +
            'Error details:' + #13 + 'Minibsdiff process cannot be created. Error: ' + IntToStr(ResultCode) + #13 + SysErrorMessage(ResultCode), mbError, MB_OK)
    else if ResultCode <> 0 then
        MsgBox('Failed to install patch ' + PatchFile + '. You have to patch the game manually!' + #13#13 +
            'Error details:' + #13 + 'Minibsdiff returned error code: ' + IntToStr(ResultCode), mbError, MB_OK)
    else
    begin
        Log('minibsdiff result code: ' + IntToStr(ResultCode));
        Result := True;
    end;
end;

function ApplyPatch(Patch: TPatchInfo): Boolean;
begin
    if Patch.PatchType = RTPatch then
        Result := ApplyRTPatch(GetGameDir(''), Patch.FileName)
    else if Patch.PatchType = BSDiff then
        Result := ApplyBSDiffPatch(GetGameDir(''), Patch.FileName, Patch.SrcFile, Patch.DestFile)
    else
        Result := False;
end;

procedure ApplyPatches;
var
    i: Integer;
begin
    if IsPatchGameTaskSelected() and (Length(Patches) > 0) then
    begin
        Log('Applying patches...');
        for i := 0 to Length(Patches) - 1 do
            if not ApplyPatch(Patches[i]) then
                break;
        Log('Finished applying patches.');
    end;
end;

// Check functions

function PatchGameTaskCheck(): Boolean;
begin
    Result := Length(Patches) > 0;
end;

// Event functions

procedure InitializeWizard;
begin
    CreateSelectGameExePage;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
    if CurStep = ssPostInstall then
        ApplyPatches;
end;
