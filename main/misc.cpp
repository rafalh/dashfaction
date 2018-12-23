#include "stdafx.h"
#include "misc.h"
#include "rf.h"
#include "version.h"
#include "utils.h"
#include "main.h"

using namespace rf;

constexpr int EGG_ANIM_ENTER_TIME = 2000;
constexpr int EGG_ANIM_LEAVE_TIME = 2000;
constexpr int EGG_ANIM_IDLE_TIME = 3000;

auto GetVersionStr_Hook = makeFunHook(GetVersionStr);
auto MenuUpdate_Hook = makeFunHook(MenuUpdate);
auto MouseUpdateDirectInput_Hook = makeFunHook(MouseUpdateDirectInput);
auto SndConvertVolume3D_AmbientSound_Hook = makeCallHook(SndConvertVolume3D);
auto PlayerLocalFireControl_Hook = makeFunHook(PlayerLocalFireControl);
auto GeomCachePrepareRoom_Hook = makeFunHook(GeomCachePrepareRoom);

int g_VersionLabelX, g_VersionLabelWidth, g_VersionLabelHeight;
static const char g_szVersionInMenu[] = PRODUCT_NAME_VERSION;
int g_VersionClickCounter = 0;
int g_EggAnimStart;
bool g_bWin32Console = false;

void __fastcall UiLabel_Create2_VersionLabel(UiPanel *pThis, void *edx, UiPanel *pParent, int x, int y, int w, int h, const char *pszText, int FontId)
{
    x = g_VersionLabelX;
    w = g_VersionLabelWidth;
    h = g_VersionLabelHeight;
    UiLabel_Create2(pThis, pParent, x, y, w, h, pszText, FontId);
}

static void GetVersionStr_New(const char **ppszVersion, const char **a2)
{
    if (ppszVersion)
        *ppszVersion = g_szVersionInMenu;
    if (a2)
        *a2 = "";
    GrGetTextWidth(&g_VersionLabelWidth, &g_VersionLabelHeight, g_szVersionInMenu, -1, g_MediumFontId);

    g_VersionLabelX = 430 - g_VersionLabelWidth;
    g_VersionLabelWidth = g_VersionLabelWidth + 5;
    g_VersionLabelHeight = g_VersionLabelHeight + 2;
}

static int MenuUpdate_New()
{
    int MenuId = MenuUpdate_Hook.callTrampoline();
    if (MenuId == MENU_MP_LIMBO) // hide cursor when changing level - hackfixed in RF by chaning rendering logic
        rf::SetCursorVisible(false);
    else if (MenuId == MENU_MAIN)
        rf::SetCursorVisible(true);
    return MenuId;
}

#ifndef __GNUC__ // FIXME
NAKED void CrashFix_0055CE48()
{
    _asm
    {
        shl   edi, 5
        lea   edx, [esp + 0x38 - 0x28]
        mov   eax, [eax + edi]
        test  eax, eax // check if pD3DTexture is NULL
        jz    CrashFix0055CE59_label1
        //jmp CrashFix0055CE59_label1
        push  0
        push  0
        push  edx
        push  0
        push  eax
        mov   ecx, 0x0055CE59
        jmp   ecx
        CrashFix0055CE59_label1 :
        mov   ecx, 0x0055CF23 // fail gr_lock
        jmp   ecx
    }
}
#endif // __GNUC__

void MenuMainProcessMouseHook()
{
    MenuMainProcessMouse();
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

void MenuMainRenderHook()
{
    MenuMainRender();
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

void SndConvertVolume3D_AmbientSound_New(int GameSndId, CVector3 *pSoundPos, float *pPanOut, float *pVolumeOut, float VolumeIn)
{
    SndConvertVolume3D_AmbientSound_Hook.callParent(GameSndId, pSoundPos, pPanOut, pVolumeOut, VolumeIn);
    *pVolumeOut *= g_gameConfig.levelSoundVolume;
}

void MouseUpdateDirectInput_New()
{
    MouseUpdateDirectInput_Hook.callTrampoline();

    // center cursor
    POINT pt = { g_GrScreen.MaxWidth / 2, g_GrScreen.MaxHeight / 2 };
    ClientToScreen(g_hWnd, &pt);
    SetCursorPos(pt.x, pt.y);
}

bool IsHoldingAssaultRifle()
{
    static auto &AssaultRifleClassId = *(int*)0x00872470;
    EntityObj *pEntity = EntityGetFromHandle((g_pLocalPlayer)->hEntity);
    return pEntity && pEntity->WeaponInfo.WeaponClsId == AssaultRifleClassId;
}

void PlayerLocalFireControl_New(CPlayer *pPlayer, bool bSecondary, char WasPressed)
{
    if (g_gameConfig.swapAssaultRifleControls && IsHoldingAssaultRifle())
        bSecondary = !bSecondary;
    PlayerLocalFireControl_Hook.callTrampoline(pPlayer, bSecondary, WasPressed);
}

char IsEntityCtrlActive_New(ControlConfig *pControlsState, EGameCtrl CtrlId, bool *pWasPressed)
{
    if (g_gameConfig.swapAssaultRifleControls && IsHoldingAssaultRifle())
    {
        if (CtrlId == GC_PRIMARY_ATTACK)
            CtrlId = GC_SECONDARY_ATTACK;
        else if (CtrlId == GC_SECONDARY_ATTACK)
            CtrlId = GC_PRIMARY_ATTACK;
    }
    return IsEntityCtrlActive(pControlsState, CtrlId, pWasPressed);
}

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

static const auto DcProcessKbd = (void(*)())0x00509F90;
static const auto DcDrawServerConsole = (void(*)())0x0050A770;
static const auto KeyGetFromQueue = (int(*)())0x0051F000;
static const auto KeyProcessEvent = (void(*)(int ScanCode, int bKeyDown, int DeltaT))0x0051E6C0;

auto DcPrint_Hook = makeFunHook(DcPrint);
auto DcDrawServerConsole_Hook = makeFunHook(DcDrawServerConsole);
auto KeyGetFromQueue_Hook = makeFunHook(KeyGetFromQueue);

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

void OsInitWindow_Server_New()
{
    AllocConsole();
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    //std::thread InputThread(InputThreadProc);
    //InputThread.detach();
}

void DcPrint_New(const char *pszText, const int *pColor)
{
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

void DcPutChar_NewLine_New()
{
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsoleA(hOutput, "\r\n", 2, NULL, NULL);
}

void DcDrawServerConsole_New()
{
    static char szPrevCmdLine[1024];
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (strncmp(g_szDcCmdLine, szPrevCmdLine, _countof(szPrevCmdLine)) != 0)
    {
        ResetConsoleCursorColumn(true);
        PrintCmdInputLine();
        strncpy(szPrevCmdLine, g_szDcCmdLine, _countof(szPrevCmdLine));
    }
}

int KeyGetFromQueue_New()
{
    if (!g_bDedicatedServer)
        return KeyGetFromQueue_Hook.callTrampoline();

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

    return KeyGetFromQueue_Hook.callTrampoline();
}

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

int GeomCachePrepareRoom_New(void *pGeom, void *pRoom)
{
    int ret = GeomCachePrepareRoom_Hook.callTrampoline(pGeom, pRoom);
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

void MiscInit()
{
    // Console init string
    WriteMemPtr(0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    // Version in Main Menu
    AsmWritter(0x0044344D).callLong(UiLabel_Create2_VersionLabel);
    GetVersionStr_Hook.hook(GetVersionStr_New);

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
#ifndef __GNUC__
    AsmWritter(0x0055CE47).jmpLong(CrashFix_0055CE48);
#endif

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
    WriteMemInt32(0x004437B9 + 1, (uintptr_t)MenuMainProcessMouseHook - (0x004437B9 + 0x5));
    WriteMemInt32(0x00443802 + 1, (uintptr_t)MenuMainRenderHook - (0x00443802 + 0x5));
#endif

    // Fix console rendering when changing level
    AsmWritter(0x0047C490).ret();
    AsmWritter(0x0047C4AA).ret();
    WriteMemUInt8(0x004B2E15, ASM_NOP, 2);
    MenuUpdate_Hook.hook(MenuUpdate_New);

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
    SndConvertVolume3D_AmbientSound_Hook.hook(0x00505F93, SndConvertVolume3D_AmbientSound_New);

    // hook MouseUpdateDirectInput
    MouseUpdateDirectInput_Hook.hook(MouseUpdateDirectInput_New);

    // Chat color alpha
    AsmWritter(0x00477490, 0x004774A4).mov(AsmRegs::EAX, 0x30); // chatbox border
    AsmWritter(0x00477528, 0x00477535).mov(AsmRegs::EBX, 0x40); // chatbox background
    AsmWritter(0x00478E00, 0x00478E14).mov(AsmRegs::EAX, 0x30); // chat input border
    AsmWritter(0x00478E91, 0x00478E9E).mov(AsmRegs::EBX, 0x40); // chat input background

    // Show enemy bullets (FIXME: add config)
    if (g_gameConfig.showEnemyBullets)
        WriteMemUInt8(0x0042669C, ASM_SHORT_JMP_REL);

    // Swap Assault Rifle fire controls
    PlayerLocalFireControl_Hook.hook(PlayerLocalFireControl_New);
    WriteMemInt32(0x00430E65 + 1, (uintptr_t)IsEntityCtrlActive_New - (0x00430E65 + 5));
    WriteMemInt32(0x00430EF7 + 1, (uintptr_t)IsEntityCtrlActive_New - (0x00430EF7 + 5));
    DC_REGISTER_CMD(swap_assault_rifle_controls, "Swap Assault Rifle controls", DcfSwapAssaultRifleControls);

    // Fix crash in shadows rendering
    WriteMemUInt8(0x0054A3C0 + 2, 16);

    // Fix crash in geometry rendering
    GeomCachePrepareRoom_Hook.hook(GeomCachePrepareRoom_New);

    // Remove Sleep calls in TimerInit
    AsmWritter(0x00504A67, 0x00504A82).nop();

#if 0
    // Fix weapon switch glitch when reloading (should be used on Match Mode)
    AsmWritter(0x004A4B4B).callLong(EntityIsReloading_SwitchWeapon_New);
    AsmWritter(0x004A4B77).callLong(EntityIsReloading_SwitchWeapon_New);
#endif

#if SERVER_WIN32_CONSOLE // win32 console
    g_bWin32Console = stristr(GetCommandLineA(), "-win32-console") != nullptr;
    if (g_bWin32Console)
    {
        WriteMemUInt32(0x004B27C5 + 1, (uintptr_t)OsInitWindow_Server_New - (0x004B27C5 + 0x5));
        //AsmWritter(0x0050A770).ret(); // null DcDrawServerConsole
        DcPrint_Hook.hook(DcPrint_New);
        DcDrawServerConsole_Hook.hook(DcDrawServerConsole_New);
        KeyGetFromQueue_Hook.hook(KeyGetFromQueue_New);
        AsmWritter(0x0050A081).callLong(DcPutChar_NewLine_New);
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
