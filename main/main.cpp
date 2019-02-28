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
#include "high_fps.h"
#include "misc.h"
#include "GameConfig.h"
#include <CallHook2.h>
#include <FunHook2.h>

#include <log/FileAppender.h>
#include <log/ConsoleAppender.h>
#include <log/Win32Appender.h>

#ifdef HAS_EXPERIMENTAL
    #include "experimental.h"
#endif

using namespace rf;

GameConfig g_gameConfig;
HMODULE g_hModule;

// Short type names
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

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
    CPlayer *pPlayer = g_pPlayersList;
    while (pPlayer)
    {
        if (Query(pPlayer->strName.psz))
            Consumer(pPlayer);
        pPlayer = pPlayer->pNext;
        if (pPlayer == g_pPlayersList)
            break;
    }
}

CallHook2<void(u8)> DcUpdate_Hook{ 0x004B2DD3,
    [](u8 bServer) {
        // Draw on top (after scene)

        DcUpdate_Hook.CallTarget(bServer);

        GraphicsDrawFpsCounter();

#ifdef LEVELS_AUTODOWNLOADER
        RenderDownloadProgress();
#endif

#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRender();
#endif
        DebugRender2d();
    } };

CallHook2<void()> InitGame_Hook{ 0x004B27CD,
    []() {
        DWORD StartTicks = GetTickCount();
        INFO("Initializing game...");

        InitGame_Hook.CallTarget();

        GraphicsAfterGameInit();

#if DIRECTINPUT_SUPPORT
        g_bDirectInputDisabled = !g_gameConfig.directInput;
#endif

        /* Allow modded strings.tbl in ui.vpp */
        ForceFileFromPackfile("strings.tbl", "ui.vpp");

        CommandsAfterGameInit();
        ScreenshotAfterGameInit();

        INFO("Game initialized (%u ms).", GetTickCount() - StartTicks);
    } };

CallHook2<void()> CleanupGame_Hook{ 0x004B2821,
    []() {
        ResetGammaRamp();
        CleanupScreenshot();
        MiscCleanup();
        CleanupGame_Hook.CallTarget();
    } };

CallHook2<bool()> RunGame_Hook{ 0x004B2818,
    []() {
        ProcessWaitingMessages();
        HighFpsUpdate();

        return RunGame_Hook.CallTarget();
    } };

CallHook2<void()> RenderInGame_Hook{ 0x00432375,
    []() {
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRenderInGame();
#endif
        DebugRender3d();
    }
};

FunHook2<int()> KeyGetFromFifo_Hook{ 0x0051F000,
    []() {
        // Process messages here because when watching videos main loop is not running
        ProcessWaitingMessages();
        return KeyGetFromFifo_Hook.CallTarget();
    } };

FunHook2<void(CPlayer*)> RenderHitScreen_Hook{ 0x004163C0,
    [](CPlayer* pPlayer) {
        RenderHitScreen_Hook.CallTarget(pPlayer);

#if SPECTATE_MODE_ENABLE
        SpectateModeDrawUI();
#endif
    } };

FunHook2<CPlayer*(char)> PlayerCreate_Hook{ 0x004A3310,
    [](char bLocal) {
        CPlayer* pPlayer = PlayerCreate_Hook.CallTarget(bLocal);
        KillInitPlayer(pPlayer);
        return pPlayer;
    } };

FunHook2<void(CPlayer*)> PlayerDestroy_Hook{ 0x004A35C0,
    [](CPlayer* pPlayer) {
        SpectateModeOnDestroyPlayer(pPlayer);
        PlayerDestroy_Hook.CallTarget(pPlayer);
    } };

CallHook2<void()> AfterFullGameInit_Hook{ 0x004B2693,
    []() {
        SpectateModeAfterFullGameInit();
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalInitAfterGame();
#endif
    } };

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
    KeyGetFromFifo_Hook.Install();

    /* General game hooks */
    InitGame_Hook.Install();
    CleanupGame_Hook.Install();
    RunGame_Hook.Install();
    RenderInGame_Hook.Install();
    AfterFullGameInit_Hook.Install();
    DcUpdate_Hook.Install();

    RenderHitScreen_Hook.Install();
    PlayerCreate_Hook.Install();
    PlayerDestroy_Hook.Install();

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
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
    ExperimentalInit();
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
