#include "stdafx.h"
#include "misc.h"
#include "rf.h"
#include "version.h"
#include "utils.h"
#include "main.h"
#include "inline_asm.h"
#include <CallHook2.h>
#include <FunHook2.h>
#include <RegsPatch.h>
#include <rfproto.h>

using namespace rf;

constexpr int EGG_ANIM_ENTER_TIME = 2000;
constexpr int EGG_ANIM_LEAVE_TIME = 2000;
constexpr int EGG_ANIM_IDLE_TIME = 3000;

int g_VersionLabelX, g_VersionLabelWidth, g_VersionLabelHeight;
static const char g_szVersionInMenu[] = PRODUCT_NAME_VERSION;
int g_VersionClickCounter = 0;
int g_EggAnimStart;
bool g_bWin32Console = false;

using UiLabel_Create2_Type = void __fastcall(UiPanel*, void*, UiPanel*, int, int, int, int, const char*, int);
extern CallHook2<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_Hook;
void __fastcall UiLabel_Create2_VersionLabel(UiPanel *pThis, void *edx, UiPanel *pParent, int x, int y, int w, int h,
    const char *pszText, int FontId)
{
    x = g_VersionLabelX;
    w = g_VersionLabelWidth;
    h = g_VersionLabelHeight;
    UiLabel_Create2_VersionLabel_Hook.CallTarget(pThis, edx, pParent, x, y, w, h, pszText, FontId);
}
CallHook2<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_Hook{ 0x0044344D, UiLabel_Create2_VersionLabel };

FunHook2<void(const char **, const char **)> GetVersionStr_Hook{ 0x004B33F0,
    [](const char **ppszVersion, const char **a2) {
        if (ppszVersion)
            *ppszVersion = g_szVersionInMenu;
        if (a2)
            *a2 = "";
        GrGetTextWidth(&g_VersionLabelWidth, &g_VersionLabelHeight, g_szVersionInMenu, -1, g_MediumFontId);

        g_VersionLabelX = 430 - g_VersionLabelWidth;
        g_VersionLabelWidth = g_VersionLabelWidth + 5;
        g_VersionLabelHeight = g_VersionLabelHeight + 2;
    }
};

FunHook2<int()> MenuUpdate_Hook{ 0x00434230,
    []() {
        int MenuId = MenuUpdate_Hook.CallTarget();
        if (MenuId == MENU_MP_LIMBO) // hide cursor when changing level - hackfixed in RF by chaning rendering logic
            rf::SetCursorVisible(false);
        else if (MenuId == MENU_MAIN)
            rf::SetCursorVisible(true);
        return MenuId;
    }};

ASM_FUNC(CrashFix_0055CE48,
// ecx - num, esi - source, ebx - dest
    ASM_I  shl   edi, 5
    ASM_I  lea   edx, [esp + 0x38 - 0x28]
    ASM_I  mov   eax, [eax + edi]
    ASM_I  test  eax, eax // check if pD3DTexture is NULL
    ASM_I  jz    ASM_LABEL(CrashFix_0055CE48_label1)
    ASM_I  push  0
    ASM_I  push  0
    ASM_I  push  edx
    ASM_I  push  0
    ASM_I  push  eax
    ASM_I  mov   ecx, 0x0055CE59
    ASM_I  jmp   ecx
    ASM_I  ASM_LABEL(CrashFix_0055CE48_label1) :
    ASM_I  mov   ecx, 0x0055CF23 // fail gr_lock
    ASM_I  jmp   ecx
)

CallHook2<void()> MenuMainProcessMouse_Hook{ 0x004437B9,
    []() {
        MenuMainProcessMouse_Hook.CallTarget();
        if (MouseWasButtonPressed(0))
        {
            int x, y, z;
            MouseGetPos(&x, &y, &z);
            UiPanel *PanelsToCheck[1] = { &g_MenuVersionLabel };
            int Matched = UiGetElementFromPos(x, y, PanelsToCheck, COUNTOF(PanelsToCheck));
            if (Matched == 0)
            {
                TRACE("Version clicked");
                ++g_VersionClickCounter;
                if (g_VersionClickCounter == 3)
                    g_EggAnimStart = GetTickCount();
            }
        }
    }
};

int LoadEasterEggImage()
{
#if 1 // from resources?
    HRSRC hRes = FindResourceA(g_hModule, MAKEINTRESOURCEA(100), RT_RCDATA);
    if (!hRes)
    {
        ERR("FindResourceA failed");
        return -1;
    }
    DWORD dwSize = SizeofResource(g_hModule, hRes);
    HGLOBAL hResData = LoadResource(g_hModule, hRes);
    if (!hResData)
    {
        ERR("LoadResource failed");
        return -1;
    }
    void *pResData = LockResource(hResData);
    if (!pResData)
    {
        ERR("LockResource failed");
        return -1;
    }

    constexpr int EASTER_EGG_SIZE = 128;

    int hbm = BmCreateUserBmap(BMPF_8888, EASTER_EGG_SIZE, EASTER_EGG_SIZE);

    SGrLockData LockData;
    if (!GrLock(hbm, 0, &LockData, 1))
        return -1;

    BmConvertFormat(LockData.pBits, (BmPixelFormat)LockData.PixelFormat, pResData, BMPF_8888, EASTER_EGG_SIZE * EASTER_EGG_SIZE);
    GrUnlock(&LockData);

    return hbm;
#else
    return BmLoad("DF_pony.tga", -1, 0);
#endif
}

CallHook2<void()> MenuMainRender_Hook{ 0x00443802,
    []() {
        MenuMainRender_Hook.CallTarget();
        if (g_VersionClickCounter >= 3)
        {
            static int PonyBitmap = LoadEasterEggImage(); // data.vpp
            if (PonyBitmap == -1)
                return;
            int w, h;
            BmGetBitmapSize(PonyBitmap, &w, &h);
            int AnimDeltaTime = GetTickCount() - g_EggAnimStart;
            int PosX = (g_GrScreen.MaxWidth - w) / 2;
            int PosY = g_GrScreen.MaxHeight - h;
            if (AnimDeltaTime < EGG_ANIM_ENTER_TIME)
                PosY += h - (int)(sinf(AnimDeltaTime / (float)EGG_ANIM_ENTER_TIME * (float)M_PI / 2.0f) * h);
            else if (AnimDeltaTime > EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME)
            {
                int LeaveDelta = AnimDeltaTime - (EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME);
                PosY += (int)((1.0f - cosf(LeaveDelta / (float)EGG_ANIM_LEAVE_TIME * (float)M_PI / 2.0f)) * h);
                if (LeaveDelta > EGG_ANIM_LEAVE_TIME)
                    g_VersionClickCounter = 0;
            }
            GrDrawImage(PonyBitmap, PosX, PosY, g_GrBitmapMaterial);
        }
    }
};

void SetPlaySoundEventsVolumeScale(float fVolScale)
{
    fVolScale = clamp(fVolScale, 0.0f, 1.0f);
    unsigned Offsets[] = {
        // Play Sound event
        0x004BA4D8, 0x004BA515, 0x004BA71C, 0x004BA759, 0x004BA609, 0x004BA5F2, 0x004BA63F,
    };
    for (int i = 0; i < COUNTOF(Offsets); ++i)
        WriteMemFloat(Offsets[i] + 1, fVolScale);
}

CallHook2<void(int, CVector3 *, float *, float *, float)> SndConvertVolume3D_AmbientSound_Hook{ 0x00505F93,
    [](int GameSndId, CVector3 *pSoundPos, float *pPanOut, float *pVolumeOut, float VolumeIn) {
        SndConvertVolume3D_AmbientSound_Hook.CallTarget(GameSndId, pSoundPos, pPanOut, pVolumeOut, VolumeIn);
        *pVolumeOut *= g_gameConfig.levelSoundVolume;
    }
};

FunHook2<void()> MouseUpdateDirectInput_Hook{ 0x0051DEB0,
    []() {
        MouseUpdateDirectInput_Hook.CallTarget();

        // center cursor
        POINT pt = { g_GrScreen.MaxWidth / 2, g_GrScreen.MaxHeight / 2 };
        ClientToScreen(g_hWnd, &pt);
        SetCursorPos(pt.x, pt.y);
    }
};

bool IsHoldingAssaultRifle()
{
    static auto &AssaultRifleClassId = *(int*)0x00872470;
    EntityObj *pEntity = EntityGetFromHandle((g_pLocalPlayer)->hEntity);
    return pEntity && pEntity->WeaponInfo.WeaponClsId == AssaultRifleClassId;
}

FunHook2<void(CPlayer *, bool, char)> PlayerLocalFireControl_Hook{ 0x004A4E80,
    [](CPlayer *pPlayer, bool bSecondary, char WasPressed)
    {
        if (g_gameConfig.swapAssaultRifleControls && IsHoldingAssaultRifle())
            bSecondary = !bSecondary;
        PlayerLocalFireControl_Hook.CallTarget(pPlayer, bSecondary, WasPressed);
    }
};

extern CallHook2<char(ControlConfig *, EGameCtrl, bool *)> IsEntityCtrlActive_Hook1;
char IsEntityCtrlActive_New(ControlConfig *pControlsState, EGameCtrl CtrlId, bool *pWasPressed)
{
    if (g_gameConfig.swapAssaultRifleControls && IsHoldingAssaultRifle())
    {
        if (CtrlId == GC_PRIMARY_ATTACK)
            CtrlId = GC_SECONDARY_ATTACK;
        else if (CtrlId == GC_SECONDARY_ATTACK)
            CtrlId = GC_PRIMARY_ATTACK;
    }
    return IsEntityCtrlActive_Hook1.CallTarget(pControlsState, CtrlId, pWasPressed);
}
CallHook2<char(ControlConfig *, EGameCtrl, bool *)> IsEntityCtrlActive_Hook1{ 0x00430E65, IsEntityCtrlActive_New };
CallHook2<char(ControlConfig *, EGameCtrl, bool *)> IsEntityCtrlActive_Hook2{ 0x00430EF7, IsEntityCtrlActive_New };

void DcfSwapAssaultRifleControls()
{
    if (g_bDcRun)
    {
        g_gameConfig.swapAssaultRifleControls = !g_gameConfig.swapAssaultRifleControls;
        g_gameConfig.save();
        DcPrintf("Swap assault rifle controls: %s", g_gameConfig.swapAssaultRifleControls ? "enabled" : "disabled");
    }
}

#if SERVER_WIN32_CONSOLE

static const auto KeyProcessEvent = (void(*)(int ScanCode, int bKeyDown, int DeltaT))0x0051E6C0;

void ResetConsoleCursorColumn(bool bClear)
{
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ScrBufInfo;
    GetConsoleScreenBufferInfo(hOutput, &ScrBufInfo);
    if (ScrBufInfo.dwCursorPosition.X == 0)
        return;
    COORD NewPos = ScrBufInfo.dwCursorPosition;
    NewPos.X = 0;
    SetConsoleCursorPosition(hOutput, NewPos);
    if (bClear)
    {
        for (int i = 0; i < ScrBufInfo.dwCursorPosition.X; ++i)
            WriteConsoleA(hOutput, " ", 1, NULL, NULL);
        SetConsoleCursorPosition(hOutput, NewPos);
    }
}

void PrintCmdInputLine()
{
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ScrBufInfo;
    GetConsoleScreenBufferInfo(hOutput, &ScrBufInfo);
    WriteConsoleA(hOutput, "] ", 2, NULL, NULL);
    unsigned Offset = std::max(0, (int)g_cchDcCmdLineLen - ScrBufInfo.dwSize.X + 3);
    WriteConsoleA(hOutput, g_szDcCmdLine + Offset, g_cchDcCmdLineLen - Offset, NULL, NULL);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType)
{
    INFO("Quiting after Console CTRL");
    static auto &Close = *(int32_t*)0x01B0D758;
    Close = 1;
    return TRUE;
}

void InputThreadProc()
{
    while (true)
    {
        INPUT_RECORD InputRecord;
        DWORD NumRead = 0;
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &InputRecord, 1, &NumRead);
    }
}

CallHook2<void()> OsInitWindow_Server_Hook{ 0x004B27C5,
    []() {
        AllocConsole();
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

        //std::thread InputThread(InputThreadProc);
        //InputThread.detach();
    }
};

FunHook2<void(const char *, const int *)> DcPrint_Hook{ reinterpret_cast<uintptr_t>(DcPrint),
    [](const char *pszText, const int *pColor) {
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        constexpr WORD RedAttr = FOREGROUND_RED | FOREGROUND_INTENSITY;
        constexpr WORD BlueAttr = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        constexpr WORD WhiteAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        constexpr WORD GrayAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        WORD CurrentAttr = 0;

        ResetConsoleCursorColumn(true);

        const char *Ptr = pszText;
        while (*Ptr)
        {
            std::string Color;
            if (Ptr[0] == '[' && Ptr[1] == '$')
            {
                const char *ColorEndPtr = strchr(Ptr + 2, ']');
                if (ColorEndPtr)
                {
                    Color.assign(Ptr + 2, ColorEndPtr - Ptr - 2);
                    Ptr = ColorEndPtr + 1;
                }
            }

            const char *EndPtr = strstr(Ptr, "[$");
            if (!EndPtr)
                EndPtr = Ptr + strlen(Ptr);

            WORD Attr;
            if (Color == "Red")
                Attr = RedAttr;
            else if (Color == "Blue")
                Attr = BlueAttr;
            else if (Color == "White")
                Attr = WhiteAttr;
            else
            {
                if (!Color.empty())
                    ERR("unknown color %s", Color.c_str());
                Attr = GrayAttr;
            }

            if (CurrentAttr != Attr)
            {
                CurrentAttr = Attr;
                SetConsoleTextAttribute(hOutput, Attr);
            }
            
            DWORD NumChars = EndPtr - Ptr;
            WriteFile(hOutput, Ptr, NumChars, NULL, NULL);
            Ptr = EndPtr;
        }

        if (Ptr > pszText && Ptr[-1] != '\n')
            WriteFile(hOutput, "\n", 1, NULL, NULL);

        if (CurrentAttr != GrayAttr)
            SetConsoleTextAttribute(hOutput, GrayAttr);

        //PrintCmdInputLine();
    }
};

CallHook2<void()> DcPutChar_NewLine_Hook{ 0x0050A081,
    [] {
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleA(hOutput, "\r\n", 2, NULL, NULL);
    }
};

FunHook2<void()> DcDrawServerConsole_Hook{ 0x0050A770,
    []() {
        static char szPrevCmdLine[1024];
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        if (strncmp(g_szDcCmdLine, szPrevCmdLine, _countof(szPrevCmdLine)) != 0)
        {
            ResetConsoleCursorColumn(true);
            PrintCmdInputLine();
            strncpy(szPrevCmdLine, g_szDcCmdLine, _countof(szPrevCmdLine));
        }
    }
};

FunHook2<int()> KeyGetFromQueue_Hook{ 0x0051F000,
    []() {
        if (!g_bDedicatedServer)
            return KeyGetFromQueue_Hook.CallTarget();

        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        INPUT_RECORD InputRecord;
        DWORD NumRead = 0;
        while (false)
        {
            if (!PeekConsoleInput(hInput, &InputRecord, 1, &NumRead) || NumRead == 0)
                break;
            if (!ReadConsoleInput(hInput, &InputRecord, 1, &NumRead) || NumRead == 0)
                break;
            if (InputRecord.EventType == KEY_EVENT)
                KeyProcessEvent(InputRecord.Event.KeyEvent.wVirtualScanCode, InputRecord.Event.KeyEvent.bKeyDown, 0);
        }

        return KeyGetFromQueue_Hook.CallTarget();
    }
};

#endif // SERVER_WIN32_CONSOLE

bool EntityIsReloading_SwitchWeapon_New(EntityObj *pEntity)
{
    if (EntityIsReloading(pEntity))
        return true;

    int WeaponClsId = pEntity->WeaponInfo.WeaponClsId;
    if (WeaponClsId >= 0)
    {
        WeaponClass *pWeaponCls = &g_pWeaponClasses[pEntity->WeaponInfo.WeaponClsId];
        if (pEntity->WeaponInfo.WeaponsAmmo[pEntity->WeaponInfo.WeaponClsId] == 0 && pEntity->WeaponInfo.ClipAmmo[pWeaponCls->AmmoType] > 0)
            return true;
    }
    return false;
}

FunHook2<int(void *, void *)> GeomCachePrepareRoom_Hook{ 0x004F0C00,
    [](void *pGeom, void *pRoom) {
        int ret = GeomCachePrepareRoom_Hook.CallTarget(pGeom, pRoom);
        char **ppRoomGeom = (char**)((char*)pRoom + 4);
        char *pRoomGeom = *ppRoomGeom;
        if (ret == 0 && pRoomGeom)
        {
            uint32_t *pRoomVertNum = (uint32_t*)(pRoomGeom + 4);
            if (*pRoomVertNum > 8000)
            {
                static int Once = 0;
                if (!(Once++))
                    WARN("Not rendering room with %u vertices!", *pRoomVertNum);
                *ppRoomGeom = NULL;
                return -1;
            }
        }
        return ret;
    }
};

struct ServerListEntry
{
  char szName[32];
  char szLevel[32];
  char szMod[16];
  int GameType;
  NwAddr Addr;
  char CurrentPlayers;
  char MaxPlayers;
  int16_t Ping;
  int field_60;
  char field_64;
  int Flags;
};
static_assert(sizeof(ServerListEntry) == 0x6C, "invalid size");

FunHook2<int (const int &Index1, const int &Index2)> ServerListCmpFunc_Hook = FunHook2{ 0x0044A6D0,
    [](const int &Index1, const int &Index2) {
        auto ServerList = AddrAsRef<ServerListEntry*>(0x0063F62C);
        bool HasPing1 = ServerList[Index1].Ping >= 0;
        bool HasPing2 = ServerList[Index2].Ping >= 0;
        if (HasPing1 != HasPing2)
            return HasPing1 ? -1 : 1;
        else
            return ServerListCmpFunc_Hook.CallTarget(Index1, Index2);
    }
};

constexpr int CHAT_MSG_MAX_LEN = 224;

FunHook2<void(uint16_t)> ChatSayAddChar_Hook{ 0x00444740,
    [](uint16_t Key) {
        if (Key)
            ChatSayAddChar_Hook.CallTarget(Key);
    }
};

FunHook2<void(const char *, bool)> ChatSayAccept_Hook{ 0x00444440,
    [](const char *Msg, bool IsTeamMsg) {
        std::string MsgStr{Msg};
        if (MsgStr.size() > CHAT_MSG_MAX_LEN)
            MsgStr.resize(CHAT_MSG_MAX_LEN);
        ChatSayAccept_Hook.CallTarget(MsgStr.c_str(), IsTeamMsg);
    }
};

constexpr uint8_t TRIGGER_CLIENT_SIDE = 0x2;
constexpr uint8_t TRIGGER_SOLO        = 0x4;
constexpr uint8_t TRIGGER_TELEPORT    = 0x8;

void SendTriggerActivatePacket(CPlayer *Player, TriggerObj *Trigger, int32_t hEntity)
{
    rfTriggerActivate Packet;
    Packet.type = RF_TRIGGER_ACTIVATE;
    Packet.size = sizeof(Packet) - sizeof(rfPacketHeader);
    Packet.uid = Trigger->_Super.Uid;
    Packet.entity_handle = hEntity;
    NwSendReliablePacket(Player, reinterpret_cast<uint8_t*>(&Packet), sizeof(Packet), 0);
}

FunHook2<void(TriggerObj*, int32_t, bool)> TriggerActivate_Hook{ 0x004C0220,
    [](TriggerObj *Trigger, int32_t hEntity, bool SkipMovers) {
        // Check team
        auto Player = GetPlayerFromEntityHandle(hEntity);
        auto TriggerName = CString_CStr(&Trigger->_Super.strName);
        if (Player && Trigger->Team != -1 && Trigger->Team != Player->bBlueTeam)
        {
            //DcPrintf("Trigger team does not match: %d vs %d (%s)", Trigger->Team, Player->bBlueTeam, TriggerName);
            return;
        }

        // Check if this is Solo or Teleport trigger (REDPF feature)
        uint8_t ExtFlags = TriggerName[0] == '\xAB' ? TriggerName[1] : 0;
        bool IsSoloTrigger = (ExtFlags & (TRIGGER_SOLO | TRIGGER_TELEPORT)) != 0;
        if (g_bNetworkGame && g_bLocalNetworkGame && IsSoloTrigger && Player)
        {
            //DcPrintf("Solo/Teleport trigger activated %s", TriggerName);
            SendTriggerActivatePacket(Player, Trigger, hEntity);
            return;
        }

        // Normal activation
        //DcPrintf("trigger normal activation %s %d", TriggerName, ExtFlags);
        TriggerActivate_Hook.CallTarget(Trigger, hEntity, SkipMovers);
    }
};

extern "C" bool IsClientSideTrigger(TriggerObj *Trigger)
{
    auto TriggerName = CString_CStr(&Trigger->_Super.strName);
    uint8_t ExtFlags = TriggerName[0] == '\xAB' ? TriggerName[1] : 0;
    return (ExtFlags & TRIGGER_CLIENT_SIDE) != 0;
}

ASM_FUNC(TriggerCheckActivation_Patch_004BFCCD,
    ASM_I  cmp  ebp, eax
    ASM_I  jz   ASM_LABEL(TriggerCheckActivation_Patch_Activate)
    ASM_I  push [esp+0x1C+0x4] // Trigger
    ASM_I  call ASM_SYM(IsClientSideTrigger)
    ASM_I  add  esp, 4
    ASM_I  jnz  ASM_LABEL(TriggerCheckActivation_Patch_Activate)

    ASM_I  pop  edi
    ASM_I  pop  esi
    ASM_I  pop  ebp
    ASM_I  push 0x004BFCD4
    ASM_I  ret

    ASM_I  ASM_LABEL(TriggerCheckActivation_Patch_Activate) :
    ASM_I  push 0x004BFCDB
    ASM_I  ret
)

RegsPatch RflLoadInternal_CheckRestoreStatus_Patch{
    0x00461195,
    [](X86Regs &Regs) {
        // check if SaveRestoreLoadAll is successful
        if (Regs.EAX) return;
        // check if this is auto-load when changing level
        const char *SaveFilename = reinterpret_cast<const char*>(Regs.EDI);
        if (!strcmp(SaveFilename, "auto.svl")) return;
        // manual load failed
        ERR("Restoring game state failed");
        char *ErrorInfo = *reinterpret_cast<char**>(Regs.ESP + 0x2B0 + 0xC);
        strcpy(ErrorInfo, "Save file is corrupted");
        // return to RflLoadInternal failure path
        Regs.EIP = 0x004608CC;
    }
};

static int g_CutsceneBackgroundSoundSig = -1;

FunHook2<void(char)> MenuInGameUpdateCutscene_Hook{
    0x0045B5E0,
    [](char bDlgOpen) {
        bool SkipCutscene = false;
        IsEntityCtrlActive(&g_pLocalPlayer->Config.Controls, GC_JUMP, &SkipCutscene);

        if (!SkipCutscene)
        {
            MenuInGameUpdateCutscene_Hook.CallTarget(bDlgOpen);
            
            GrSetColor(255, 255, 255, 255);
            GrDrawAlignedText(GR_ALIGN_CENTER, g_GrScreen.MaxWidth / 2, g_GrScreen.MaxHeight - 30,
                "Press JUMP key to skip the cutscene", -1, rf::g_GrTextMaterial);
        }
        else
        {
            auto CutsceneIsActive = (bool(*)()) 0x0045BE80;
            auto TimerAddDeltaTime = (int(*)(int DeltaMs)) 0x004FA2D0;
            auto SndStop = (char(*)(int Sig)) 0x005442B0;
            auto Timer__GetTimeLeftMs = (int (__thiscall*)(void *Timer)) 0x004FA420;

            auto &FrameTime = AddrAsRef<float>(0x005A4014);
            auto &ActiveCutscene = AddrAsRef<void*>(0x00645320);
            auto &CurrentShotIdx = StructFieldRef<int>(ActiveCutscene, 0x808);
            auto &NumShots = StructFieldRef<int>(ActiveCutscene, 4);

            if (g_CutsceneBackgroundSoundSig != -1)
            {
                SndStop(g_CutsceneBackgroundSoundSig);
                g_CutsceneBackgroundSoundSig = -1;
            }
            while (CutsceneIsActive())
            {
                void *CurrentShotTimer = reinterpret_cast<char*>(ActiveCutscene) + 0x810;
                int ShotTimeLeft = Timer__GetTimeLeftMs(CurrentShotTimer);
                // run the frame when cutscene ends with normal speed so nothing weird happens
                if (CurrentShotIdx == NumShots - 1 && ShotTimeLeft > 100)
                    ShotTimeLeft -= 10;
                TimerAddDeltaTime(ShotTimeLeft);
                FrameTime = ShotTimeLeft / 1000.0f;
                MenuInGameUpdateCutscene_Hook.CallTarget(bDlgOpen);
            }
        }
    }
};

CallHook2<int()> PlayHardcodedBackgroundMusicForCutscene_Hook {
    0x0045BB85,
    []() {
        g_CutsceneBackgroundSoundSig = PlayHardcodedBackgroundMusicForCutscene_Hook.CallTarget();
        return g_CutsceneBackgroundSoundSig;
    }
};

void MiscInit()
{
    // Console init string
    WriteMemPtr(0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    // Version in Main Menu
    UiLabel_Create2_VersionLabel_Hook.Install();
    GetVersionStr_Hook.Install();

    // Window title (client and server)
    WriteMemPtr(0x004B2790, PRODUCT_NAME);
    WriteMemPtr(0x004B27A4, PRODUCT_NAME);

    // Console background color
    WriteMemUInt32(0x005098D1, CONSOLE_BG_A); // Alpha
    WriteMemUInt8(0x005098D6, CONSOLE_BG_B); // Blue
    WriteMemUInt8(0x005098D8, CONSOLE_BG_G); // Green
    WriteMemUInt8(0x005098DA, CONSOLE_BG_R); // Red

#ifdef NO_CD_FIX
    // No-CD fix
    WriteMemUInt8(0x004B31B6, ASM_SHORT_JMP_REL);
#endif // NO_CD_FIX

#ifdef NO_INTRO
    // Disable thqlogo.bik
    if (g_gameConfig.fastStart)
    {
        WriteMemUInt8(0x004B208A, ASM_SHORT_JMP_REL);
        WriteMemUInt8(0x004B24FD, ASM_SHORT_JMP_REL);
    }
#endif // NO_INTRO

    // Sound loop fix
    WriteMemUInt8(0x00505D08, 0x00505D5B - (0x00505D07 + 0x2));

    // Set initial FPS limit
    WriteMemFloat(0x005094CA, 1.0f / g_gameConfig.maxFps);

    // Crash-fix... (probably argument for function is invalid); Page Heap is needed
    WriteMemUInt32(0x0056A28C + 1, 0);

    // Crash-fix in case texture has not been created (this happens if GrReadBackbuffer fails)
    AsmWritter(0x0055CE47).jmpLong(CrashFix_0055CE48);

    // Dont overwrite player name and prefered weapons when loading saved game
    AsmWritter(0x004B4D99, 0x004B4DA5).nop();
    AsmWritter(0x004B4E0A, 0x004B4E22).nop();

    // Dont filter levels for DM and TeamDM
    WriteMemUInt8(0x005995B0, 0);
    WriteMemUInt8(0x005995B8, 0);

#if DIRECTINPUT_SUPPORT
    g_bDirectInputDisabled = 0;
#endif

#if 1
    // Buffer overflows in RflReadStaticGeometry
    // Note: Buffer size is 1024 but opcode allows only 1 byte size
    //       What is more important BmLoad copies texture name to 32 bytes long buffers
    WriteMemInt8(0x004ED612 + 1, 32);
    WriteMemInt8(0x004ED66E + 1, 32);
    WriteMemInt8(0x004ED72E + 1, 32);
    WriteMemInt8(0x004EDB02 + 1, 32);
#endif

#if 1 // Version Easter Egg
    MenuMainProcessMouse_Hook.Install();
    MenuMainRender_Hook.Install();
#endif

    // Fix console rendering when changing level
    AsmWritter(0x0047C490).ret();
    AsmWritter(0x0047C4AA).ret();
    WriteMemUInt8(0x004B2E15, ASM_NOP, 2);
    MenuUpdate_Hook.Install();

    // Increase damage for kill command in Single Player
    WriteMemFloat(0x004A4DF5 + 1, 100000.0f);
    
    // Fix keyboard layout
    uint8_t KbdLayout = 0;
    if (MapVirtualKeyA(0x10, MAPVK_VSC_TO_VK) == 'A')
        KbdLayout = 2; // AZERTY
    else if (MapVirtualKeyA(0x15, MAPVK_VSC_TO_VK) == 'Z')
        KbdLayout = 3; // QWERTZ
    INFO("Keyboard layout: %u", KbdLayout);
    WriteMemUInt8(0x004B14B4 + 1, KbdLayout);

    // Level sounds
    SetPlaySoundEventsVolumeScale(g_gameConfig.levelSoundVolume);
    SndConvertVolume3D_AmbientSound_Hook.Install();

    // hook MouseUpdateDirectInput
    MouseUpdateDirectInput_Hook.Install();

    // Chat color alpha
    AsmWritter(0x00477490, 0x004774A4).mov(AsmRegs::EAX, 0x30); // chatbox border
    AsmWritter(0x00477528, 0x00477535).mov(AsmRegs::EBX, 0x40); // chatbox background
    AsmWritter(0x00478E00, 0x00478E14).mov(AsmRegs::EAX, 0x30); // chat input border
    AsmWritter(0x00478E91, 0x00478E9E).mov(AsmRegs::EBX, 0x40); // chat input background

    // Show enemy bullets (FIXME: add config)
    if (g_gameConfig.showEnemyBullets)
        WriteMemUInt8(0x0042669C, ASM_SHORT_JMP_REL);

    // Swap Assault Rifle fire controls
    PlayerLocalFireControl_Hook.Install();
    IsEntityCtrlActive_Hook1.Install();
    IsEntityCtrlActive_Hook2.Install();
    DC_REGISTER_CMD(swap_assault_rifle_controls, "Swap Assault Rifle controls", DcfSwapAssaultRifleControls);

    // Fix crash in shadows rendering
    WriteMemUInt8(0x0054A3C0 + 2, 16);

    // Fix crash in geometry rendering
    GeomCachePrepareRoom_Hook.Install();

    // Remove Sleep calls in TimerInit
    AsmWritter(0x00504A67, 0x00504A82).nop();

    // Use spawnpoint team property in TeamDM game (PF compatible)
    WriteMemUInt8(0x00470395 + 4, 0); // change cmp argument: CTF -> DM
    WriteMemUInt8(0x0047039A, 0x74); // invert jump condition: jnz -> jz

    // Put not responding servers at the bottom of server list
    ServerListCmpFunc_Hook.Install();

    // Disable broken optimization of segment vs geometry collision test
    // Fixes hitting objects if mover is in the line of the shot
    AsmWritter(0x00499055).jmpLong(0x004990B4);

    // Disable Flamethower debug sphere drawing (optimization)
    // It is not visible in game because other things are drawn over it
    AsmWritter(0x0041AE47, 0x0041AE4C).nop();

    // Fix game beeping every frame if chat input buffer is full
    ChatSayAddChar_Hook.Install();

    // Change chat input limit to 224 (RF can support 255 safely but PF kicks if message is longer than 224)
    WriteMemInt32(0x0044474A + 1, CHAT_MSG_MAX_LEN);

    // Add chat message limit for say/teamsay commands
    ChatSayAccept_Hook.Install();

    // Preserve password case when processing rcon_request command
    WriteMemInt8(0x0046C85A + 1, 1);

    // Solo/Teleport triggers handling + filtering by team ID
    TriggerActivate_Hook.Install();

    // Client-side trigger flag handling
    AsmWritter(0x004BFCCD, 0x004BFCD4).jmpLong(TriggerCheckActivation_Patch_004BFCCD);

    // Fix crash when loading savegame with missing player entity data
    AsmWritter(0x004B4B47).jmpNear(0x004B4B7B);

    // Add checking if restoring game state from save file failed during level loading
    RflLoadInternal_CheckRestoreStatus_Patch.Install();

    // Support skipping cutscenes
    MenuInGameUpdateCutscene_Hook.Install();
    PlayHardcodedBackgroundMusicForCutscene_Hook.Install();

#if 0
    // Fix weapon switch glitch when reloading (should be used on Match Mode)
    AsmWritter(0x004A4B4B).callLong(EntityIsReloading_SwitchWeapon_New);
    AsmWritter(0x004A4B77).callLong(EntityIsReloading_SwitchWeapon_New);
#endif

#if SERVER_WIN32_CONSOLE // win32 console
    g_bWin32Console = stristr(GetCommandLineA(), "-win32-console") != nullptr;
    if (g_bWin32Console)
    {
        OsInitWindow_Server_Hook.Install();
        //AsmWritter(0x0050A770).ret(); // null DcDrawServerConsole
        DcPrint_Hook.Install();
        DcDrawServerConsole_Hook.Install();
        KeyGetFromQueue_Hook.Install();
        DcPutChar_NewLine_Hook.Install();
    }
#endif
}

void MiscCleanup()
{
#if SERVER_WIN32_CONSOLE // win32 console
    if (g_bWin32Console)
        FreeConsole();
#endif
}
