#include "stdafx.h"
#include "exports.h"
#include "version.h"
#include "crashdump.h"
#include "autodl.h"
#include "utils.h"
#include "rf.h"
#include "scoreboard.h"
#include "hud.h"
#include "packfile.h"
#include "lazyban.h"
#include "gamma.h"
#include "kill.h"
#include "screenshot.h"
#include "commands.h"
#include "spectate_mode.h"
#include "wndproc.h"
#include "graphics.h"
#include "network.h"
#include "test.h"
#include "high_fps.h"
#include "HookableFunPtr.h"
#include "GameConfig.h"

#include <log/FileAppender.h>
#include <log/ConsoleAppender.h>
#include <log/Win32Appender.h>

using namespace rf;

GameConfig g_gameConfig;
HMODULE g_hModule;
HookableFunPtr<0x004163C0, void, CPlayer*> RenderHitScreenHookable;
HookableFunPtr<0x004B33F0, void, const char**, const char**> GetVersionStrHookable;
HookableFunPtr<0x0051F000, int> KeyGetFromFifoHookable;
int g_VersionLabelX, g_VersionLabelWidth, g_VersionLabelHeight;
static const char g_szVersionInMenu[] = PRODUCT_NAME_VERSION;

static void ProcessWaitingMessages()
{
    // Note: When using dedicated server we get WM_PAINT messages all the time
    MSG msg;
    constexpr int LIMIT = 4;
    for (int i = 0; i < LIMIT; ++i)
    {
        if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        //INFO("msg %u\n", msg.message);
    }
}

static void DrawConsoleAndProcessKbdFifoHook(BOOL bServer)
{
    // Draw on top (after scene)

    DrawConsoleAndProcessKbdFifo(bServer);
    
    GraphicsDrawFpsCounter();
    
#ifdef LEVELS_AUTODOWNLOADER
    RenderDownloadProgress();
#endif
}

NAKED void VersionLabelPushArgs_0044343A()
{
    _asm
    {
        // Default: 330, 340, 120, 15
        push    g_VersionLabelHeight
        push    g_VersionLabelWidth
        push    340
        push    g_VersionLabelX
        mov eax, 0x00443448
        jmp eax
    }
}

static void GetVersionStrHook(const char **ppszVersion, const char **a2)
{
    if (ppszVersion)
        *ppszVersion = g_szVersionInMenu;
    if (a2)
        *a2 = "";
    GrGetTextWidth(&g_VersionLabelWidth, &g_VersionLabelHeight, g_szVersionInMenu, -1, *g_pMediumFontId);

    g_VersionLabelX = 430 - g_VersionLabelWidth;
    g_VersionLabelWidth = g_VersionLabelWidth + 5;
    g_VersionLabelHeight = g_VersionLabelHeight + 2;
}

static void InitGameHook(void)
{
    INFO("Initializing game...");

    rf::InitGame();

    GraphicsAfterGameInit();

#if DIRECTINPUT_SUPPORT
    *g_pbDirectInputDisabled = !g_gameConfig.directInput;
#endif

    /* Allow modded strings.tbl in ui.vpp */
    ForceFileFromPackfile("strings.tbl", "ui.vpp");
    
    CommandsAfterGameInit();
    ScreenshotAfterGameInit();

    INFO("Game initialized.");
}

static void CleanupGameHook(void)
{
    ResetGammaRamp();
    CleanupScreenshot();
    rf::CleanupGame();
}

static bool RunGameHook()
{
    ProcessWaitingMessages();
    HighFpsUpdate();

    return rf::RunGame();
}

int KeyGetFromFifoHook()
{
    // Process messages here because when watching videos main loop is not running
    ProcessWaitingMessages();
    return KeyGetFromFifoHookable.callOrig();
}

CPlayer *FindPlayer(const char *pszName)
{
    CPlayer *pPlayer = *g_ppPlayersList;
    while(pPlayer)
    {
        if(stristr(pPlayer->strName.psz, pszName))
            return pPlayer;
        pPlayer = pPlayer->pNext;
        if(pPlayer == *g_ppPlayersList)
            break;
    }
    return NULL;
}

static void RenderHitScreenHook(CPlayer *pPlayer)
{
    RenderHitScreenHookable.callOrig(pPlayer);

#if SPECTATE_MODE_ENABLE
    SpectateModeDrawUI();
#endif

#ifndef NDEBUG
    TestRender();
#endif
}

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
    CrashFix0055CE59_label1:
        mov   ecx, 0x0055CF23 // fail gr_lock
        jmp   ecx
    }
}

#ifndef NDEBUG
class RfConsoleLogAppender : public logging::BaseAppender
{
    virtual void append(logging::LogLevel lvl, const std::string &str)
    {
        RfConsoleWrite(str.c_str(), NULL);
    }
};
#endif // NDEBUG

void InitLogging()
{
    CreateDirectoryA("logs", NULL);
    logging::LoggerConfig::root().addAppender(std::move(std::make_unique<logging::FileAppender>("logs/DashFaction.log", false)));
    logging::LoggerConfig::root().addAppender(std::move(std::make_unique<logging::ConsoleAppender>()));
    logging::LoggerConfig::root().addAppender(std::move(std::make_unique<logging::Win32Appender>()));
#ifndef NDEBUG
    logging::LoggerConfig::root().addAppender(std::move(std::make_unique<RfConsoleLogAppender>()));
#endif
}

void LogSystemInfo()
{
    try
    {
        std::string osVer = getRealOsVersion();
        INFO("Real system version: %s", osVer.c_str());
        osVer = getOsVersion();
        INFO("Emulated system version: %s", osVer.c_str());
        INFO("Running as %s (elevation type: %s)", IsUserAdmin() ? "admin" : "user", GetProcessElevationType());
    }
    catch (std::exception &e)
    {
        ERR("Failed to read system info: %s", e.what());
    }
}

extern "C" DWORD DLL_EXPORT Init(void *pUnused)
{
    // Init logging and crash dump support first
    InitLogging();
    InitCrashDumps();
    
    // Enable Data Execution Prevention
    if (!SetProcessDEPPolicy(PROCESS_DEP_ENABLE))
        WARN("SetProcessDEPPolicy failed (error %ld)", GetLastError());

    // Log system info
    LogSystemInfo();

    // Load config
    try
    {
        if (!g_gameConfig.load())
            ERR("Configuration has not been found in registry!");
    }
    catch (std::exception &e)
    {
        ERR("Failed to load configuration: %s", e.what());
    }

    // Log information from config
    INFO("Resolution: %dx%dx%d", g_gameConfig.resWidth, g_gameConfig.resHeight, g_gameConfig.resBpp);
    INFO("Window Mode: %d", (int)g_gameConfig.wndMode);
    INFO("Max FPS: %d", (int)g_gameConfig.maxFps);
    INFO("Allow Overwriting Game Files: %d", (int)g_gameConfig.allowOverwriteGameFiles);

    /* Process messages in the same thread as DX processing (alternative: D3DCREATE_MULTITHREADED) */
    WriteMemUInt8(0x00524C48, ASM_NOP, 0x00524C83 - 0x00524C48); // disable msg loop thread
    WriteMemUInt8(0x00524C48, ASM_LONG_CALL_REL);
    WriteMemUInt32(0x00524C49, 0x00524E40 - (0x00524C48 + 0x5)); // CreateMainWindow
    KeyGetFromFifoHookable.hook(KeyGetFromFifoHook);

    /* Console init string */
    WriteMemPtr(0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    /* Version in Main Menu */
    WriteMemUInt8(0x0044343A, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0044343A + 1, (uintptr_t)VersionLabelPushArgs_0044343A - (0x0044343A + 0x5));
    GetVersionStrHookable.hook(GetVersionStrHook);
    
    /* Window title (client and server) */
    WriteMemPtr(0x004B2790, PRODUCT_NAME);
    WriteMemPtr(0x004B27A4, PRODUCT_NAME);
    
    /* Console background color */
    WriteMemUInt32(0x005098D1, CONSOLE_BG_A); // Alpha
    WriteMemUInt8(0x005098D6, CONSOLE_BG_B); // Blue
    WriteMemUInt8(0x005098D8, CONSOLE_BG_G); // Green
    WriteMemUInt8(0x005098DA, CONSOLE_BG_R); // Red

#ifdef NO_CD_FIX
    /* No-CD fix */
    WriteMemUInt8(0x004B31B6, ASM_SHORT_JMP_REL);
#endif /* NO_CD_FIX */

#ifdef NO_INTRO
    /* Disable thqlogo.bik */
    if (g_gameConfig.fastStart)
    {
        WriteMemUInt8(0x004B208A, ASM_SHORT_JMP_REL);
        WriteMemUInt8(0x004B24FD, ASM_SHORT_JMP_REL);
    }
#endif /* NO_INTRO */
    
    /* Sound loop fix */
    WriteMemUInt8(0x00505D08, 0x00505D5B - (0x00505D07 + 0x2));
    
    /* DrawConsoleAndProcssKbdFifo hook */
    WriteMemInt32(0x004B2DD3 + 1, (uintptr_t)DrawConsoleAndProcessKbdFifoHook - (0x004B2DD3 + 0x5));

    /* CleanupGame and InitGame hooks */
    WriteMemInt32(0x004B27CD + 1, (uintptr_t)InitGameHook - (0x004B27CD + 0x5));
    WriteMemInt32(0x004B2821 + 1, (uintptr_t)CleanupGameHook - (0x004B2821 + 0x5));
    WriteMemInt32(0x004B2818 + 1, (uintptr_t)RunGameHook - (0x004B2818 + 0x5));
    
    /* Set initial FPS limit */
    WriteMemFloat(0x005094CA, 1.0f / g_gameConfig.maxFps);
    
    /* Crash-fix... (probably argument for function is invalid); Page Heap is needed */
    WriteMemUInt32(0x0056A28C + 1, 0);

    /* Crash-fix in case texture has not been created (this happens if GrReadBackbuffer fails) */
    WriteMemUInt8(0x0055CE47, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0055CE47 + 1, (uintptr_t)CrashFix_0055CE48 - (0x0055CE47 + 0x5));
    
    // Dont overwrite player name and prefered weapons when loading saved game
    WriteMemUInt8(0x004B4D99, ASM_NOP, 0x004B4DA5 - 0x004B4D99);
    WriteMemUInt8(0x004B4E0A, ASM_NOP, 0x004B4E22 - 0x004B4E0A);

    RenderHitScreenHookable.hook(RenderHitScreenHook);

#if DIRECTINPUT_SUPPORT
    *g_pbDirectInputDisabled = 0;
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

    /* Init modules */
    GraphicsInit();
    NetworkInit();
    InitGamma();
    InitWndProc();
    InitScreenshot();
    InitHud();
#ifdef LEVELS_AUTODOWNLOADER
    InitAutodownloader();
#endif
    InitScoreboard();
    InitLazyban();
    InitKill();
    VfsApplyHooks(); /* Use new VFS without file count limit */
    SpectateModeInit();
    CommandsInit();
    HighFpsInit();
#ifndef NDEBUG
    TestInit();
#endif

    return 1; /* success */
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
    g_hModule = hInstance;
    DisableThreadLibraryCalls(hInstance);
    return TRUE;
}
