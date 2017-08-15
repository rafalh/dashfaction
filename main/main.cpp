#include "stdafx.h"
#include "exports.h"
#include "crashhandler.h"
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
#include "misc.h"
#include "GameConfig.h"
#include "CallHook.h"

#include <log/FileAppender.h>
#include <log/ConsoleAppender.h>
#include <log/Win32Appender.h>

using namespace rf;

GameConfig g_gameConfig;
HMODULE g_hModule;

auto InitGame_Hook = makeCallHook(InitGame);
auto CleanupGame_Hook = makeCallHook(CleanupGame);
auto RunGame_Hook = makeCallHook(RunGame);
auto DcUpdate_Hook = makeCallHook(DcUpdate);

auto RenderHitScreen_Hook = makeFunHook(RenderHitScreen);
auto PlayerCreate_Hook = makeFunHook(PlayerCreate);
auto PlayerDestroy_Hook = makeFunHook(PlayerDestroy);
auto KeyGetFromFifo_Hook = makeFunHook(KeyGetFromFifo);

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

void FindPlayer(const StringMatcher &Query, std::function<void(CPlayer*)> Consumer)
{
    CPlayer *pPlayer = *g_ppPlayersList;
    while (pPlayer)
    {
        if (Query(pPlayer->strName.psz))
            Consumer(pPlayer);
        pPlayer = pPlayer->pNext;
        if (pPlayer == *g_ppPlayersList)
            break;
    }
}

static void DcUpdate_New(BOOL bServer)
{
    // Draw on top (after scene)

    DcUpdate_Hook.callParent(bServer);
    
    GraphicsDrawFpsCounter();
    
#ifdef LEVELS_AUTODOWNLOADER
    RenderDownloadProgress();
#endif

#ifndef NDEBUG
    TestRender();
#endif
}

static void InitGame_New(void)
{
    DWORD StartTicks = GetTickCount();
    INFO("Initializing game...");

    InitGame_Hook.callParent();

    GraphicsAfterGameInit();

#if DIRECTINPUT_SUPPORT
    *g_pbDirectInputDisabled = !g_gameConfig.directInput;
#endif

    /* Allow modded strings.tbl in ui.vpp */
    ForceFileFromPackfile("strings.tbl", "ui.vpp");
    
    CommandsAfterGameInit();
    ScreenshotAfterGameInit();

    INFO("Game initialized (%u ms).", GetTickCount() - StartTicks);
}

static void CleanupGame_New(void)
{
    ResetGammaRamp();
    CleanupScreenshot();
    MiscCleanup();
    CleanupGame_Hook.callParent();
}

static bool RunGame_New()
{
    ProcessWaitingMessages();
    HighFpsUpdate();

    return RunGame_Hook.callParent();
}

void RenderInGameHook()
{
#ifdef DEBUG
    TestRenderInGame();
#endif
}

int KeyGetFromFifo_New()
{
    // Process messages here because when watching videos main loop is not running
    ProcessWaitingMessages();
    return KeyGetFromFifo_Hook.callTrampoline();
}

static void RenderHitScreen_New(CPlayer *pPlayer)
{
    RenderHitScreen_Hook.callTrampoline(pPlayer);

#if SPECTATE_MODE_ENABLE
    SpectateModeDrawUI();
#endif
}

static CPlayer *PlayerCreate_New(char bLocal)
{
    CPlayer *pPlayer = PlayerCreate_Hook.callTrampoline(bLocal);
    KillInitPlayer(pPlayer);
    return pPlayer;
}

static void PlayerDestroy_New(CPlayer *pPlayer)
{
    SpectateModeOnDestroyPlayer(pPlayer);
    PlayerDestroy_Hook.callTrampoline(pPlayer);
}

void AfterFullGameInit()
{
    SpectateModeAfterFullGameInit();
#ifndef NDEBUG
    TestInitAfterGame();
#endif
}

#ifndef NDEBUG
class RfConsoleLogAppender : public logging::BaseAppender
{
    virtual void append(logging::LogLevel lvl, const std::string &str)
    {
        DcPrint(str.c_str(), NULL);
    }
};
#endif // NDEBUG

void InitLogging()
{
    CreateDirectoryA("logs", NULL);
    logging::LoggerConfig::root().addAppender(std::move(std::make_unique<logging::FileAppender>("logs/DashFaction.log", false)));
    //logging::LoggerConfig::root().addAppender(std::move(std::make_unique<logging::ConsoleAppender>()));
    logging::LoggerConfig::root().addAppender(std::move(std::make_unique<logging::Win32Appender>()));
#ifndef NDEBUG
    logging::LoggerConfig::root().addAppender(std::move(std::make_unique<RfConsoleLogAppender>()));
#endif
}

void LogSystemInfo()
{
    try
    {
        logging::Logger::root().info() << "Real system version: " << getRealOsVersion();
        logging::Logger::root().info() << "Emulated system version: " << getOsVersion();
        INFO("Running as %s (elevation type: %s)", IsUserAdmin() ? "admin" : "user", GetProcessElevationType());
        logging::Logger::root().info() << "CPU Brand: " << getCpuBrand();
        logging::Logger::root().info() << "CPU ID: " << getCpuId();
    }
    catch (std::exception &e)
    {
        ERR("Failed to read system info: %s", e.what());
    }
}

extern "C" void subhook_unk_opcode_handler(uint8_t *opcode)
{
    ERR("SubHook unknown opcode 0x%X at 0x%p", *opcode, opcode);
}

extern "C" DWORD DLL_EXPORT Init(void *pUnused)
{
    DWORD StartTicks = GetTickCount();

    // Init logging and crash dump support first
    InitLogging();
    CrashHandlerInit(g_hModule);
    
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
    AsmWritter(0x00524C48, 0x00524C83).nop(); // disable msg loop thread
    AsmWritter(0x00524C48).callLong(0x00524E40); // CreateMainWindow
    KeyGetFromFifo_Hook.hook(KeyGetFromFifo_New);

    /* General game hooks */
    InitGame_Hook.hook(0x004B27CD, InitGame_New);
    CleanupGame_Hook.hook(0x004B2821, CleanupGame_New);
    RunGame_Hook.hook(0x004B2818, RunGame_New);
    WriteMemInt32(0x00432375 + 1, (uintptr_t)RenderInGameHook - (0x00432375 + 0x5));
    WriteMemInt32(0x004B2693 + 1, (uintptr_t)AfterFullGameInit - (0x004B2693 + 0x5));
    DcUpdate_Hook.hook(0x004B2DD3, DcUpdate_New);

    RenderHitScreen_Hook.hook(RenderHitScreen_New);
    PlayerCreate_Hook.hook(PlayerCreate_New);
    PlayerDestroy_Hook.hook(PlayerDestroy_New);

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
    MiscInit();
#ifndef NDEBUG
    TestInit();
#endif

    INFO("Installing hooks took %u ms", GetTickCount() - StartTicks);

    return 1; /* success */
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
    g_hModule = hInstance;
    DisableThreadLibraryCalls(hInstance);
    return TRUE;
}
