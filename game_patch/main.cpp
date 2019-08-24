#include <common/GameConfig.h>
#include "autodl.h"
#include "commands.h"
#include "crashhandler.h"
#include "exports.h"
#include "graphics/gamma.h"
#include "graphics/graphics.h"
#include "graphics/capture.h"
#include "high_fps.h"
#include "hud.h"
#include "kill.h"
#include "misc.h"
#include "network.h"
#include "packfile.h"
#include "rf.h"
#include "scoreboard.h"
#include "spectate_mode.h"
#include "stdafx.h"
#include "utils.h"
#include "wndproc.h"
#include "server/server.h"
#include "input/input.h"
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>

#include <log/ConsoleAppender.h>
#include <log/FileAppender.h>
#include <log/Win32Appender.h>

#ifdef HAS_EXPERIMENTAL
#include "experimental.h"
#endif

GameConfig g_game_config;
HMODULE g_hmodule;

static void ProcessWaitingMessages()
{
    // Note: When using dedicated server we get WM_PAINT messages all the time
    MSG msg;
    constexpr int limit = 4;
    for (int i = 0; i < limit; ++i) {
        if (!PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // INFO("msg %u\n", msg.message);
    }
}

void FindPlayer(const StringMatcher& query, std::function<void(rf::Player*)> consumer)
{
    rf::Player* player = rf::player_list;
    while (player) {
        if (query(player->name))
            consumer(player);
        player = player->next;
        if (player == rf::player_list)
            break;
    }
}

CallHook<void(bool)> DcUpdate_hook{
    0x004B2DD3,
    [](bool is_server) {
        // Draw on top (after scene)

        DcUpdate_hook.CallTarget(is_server);

        GraphicsDrawFpsCounter();

        RenderDownloadProgress();

#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRender();
#endif
        DebugRender2d();
    },
};

CallHook<void()> InitGame_hook{
    0x004B27CD,
    []() {
        DWORD start_ticks = GetTickCount();
        INFO("Initializing game...");

        InitGame_hook.CallTarget();

        GraphicsAfterGameInit();

        CommandsAfterGameInit();
        GraphicsCaptureAfterGameInit();

        INFO("Game initialized (%lu ms).", GetTickCount() - start_ticks);
    },
};

CodeInjection after_gs_init_hook{
    0x004B26C6,
    []([[ maybe_unused ]] auto& regs) {
        INFO("Game fully initialized");
    },
};

CallHook<void()> CleanupGame_hook{
    0x004B2821,
    []() {
        ResetGammaRamp();
        MiscCleanup();
        ServerCleanup();
        CleanupGame_hook.CallTarget();
    },
};

CallHook<bool()> RunGame_hook{
    0x004B2818,
    []() {
        ProcessWaitingMessages();
        HighFpsUpdate();

        return RunGame_hook.CallTarget();
    },
};

CallHook<void()> RenderInGame_hook{
    0x00432375,
    []() {
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRenderInGame();
#endif
        DebugRender3d();
    },
};

FunHook<int()> KeyGetFromFifo_hook{
    0x0051F000,
    []() {
        // Process messages here because when watching videos main loop is not running
        ProcessWaitingMessages();
        return KeyGetFromFifo_hook.CallTarget();
    },
};

FunHook<void(rf::Player*)> RenderHitScreen_hook{
    0x004163C0,
    [](rf::Player* player) {
        RenderHitScreen_hook.CallTarget(player);

        if (rf::is_net_game)
            SpectateModeDrawUI();
    },
};

FunHook<rf::Player*(bool)> PlayerCreate_hook{
    0x004A3310,
    [](bool is_local) {
        rf::Player* player = PlayerCreate_hook.CallTarget(is_local);
        KillInitPlayer(player);
        return player;
    },
};

FunHook<void(rf::Player*)> PlayerDestroy_hook{
    0x004A35C0,
    [](rf::Player* player) {
        SpectateModeOnDestroyPlayer(player);
        PlayerDestroy_hook.CallTarget(player);
    },
};

CallHook<void()> AfterFullGameInit_hook{
    0x004B2693,
    []() {
        SpectateModeAfterFullGameInit();
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalInitAfterGame();
#endif
    },
};

#ifndef NDEBUG
class RfConsoleLogAppender : public logging::BaseAppender
{
    const int m_error_color = 0xFF0000;
    std::vector<std::pair<std::string, logging::Level>> m_startup_buf;

    virtual void append([[maybe_unused]] logging::Level lvl, const std::string& str) override
    {
        auto& console_inited = AddrAsRef<bool>(0x01775680);
        if (console_inited) {
            for (auto& p : m_startup_buf) {
                uint32_t color = color_from_level(p.second);
                rf::DcPrint(p.first.c_str(), &color);
            }
            m_startup_buf.clear();

            uint32_t color = color_from_level(lvl);
            rf::DcPrint(str.c_str(), &color);
        }
        else {
            m_startup_buf.push_back({str, lvl});
        }
    }

    uint32_t color_from_level(logging::Level lvl) const
    {
        switch (lvl) {
            case logging::Level::fatal:
                return 0xFF0000FF;
            case logging::Level::error:
                return 0xFF0000FF;
            case logging::Level::warning:
                return 0xFF00FFFF;
            case logging::Level::info:
                return 0xFFAAAAAA;
            case logging::Level::trace:
                return 0xFF888888;
            default:
                return 0xFFFFFFFF;
        }
    }
};
#endif // NDEBUG

void InitLogging()
{
    CreateDirectoryA("logs", nullptr);
    auto& logger_config = logging::LoggerConfig::root();
    logger_config.add_appender(std::make_unique<logging::FileAppender>("logs/DashFaction.log", false));
    logger_config.add_appender(std::make_unique<logging::ConsoleAppender>());
    logger_config.add_appender(std::make_unique<logging::Win32Appender>());
#ifndef NDEBUG
    logger_config.add_appender(std::make_unique<RfConsoleLogAppender>());
#endif
}

void LogSystemInfo()
{
    try {
        logging::Logger::root().info() << "Real system version: " << getRealOsVersion();
        logging::Logger::root().info() << "Emulated system version: " << getOsVersion();
        INFO("Running as %s (elevation type: %s)", IsUserAdmin() ? "admin" : "user", GetProcessElevationType());
        logging::Logger::root().info() << "CPU Brand: " << getCpuBrand();
        logging::Logger::root().info() << "CPU ID: " << getCpuId();
    }
    catch (std::exception& e) {
        ERR("Failed to read system info: %s", e.what());
    }
}

extern "C" void subhook_unk_opcode_handler(uint8_t* opcode)
{
    ERR("SubHook unknown opcode 0x%X at 0x%p", *opcode, opcode);
}

extern "C" DWORD DF_DLL_EXPORT Init([[maybe_unused]] void* unused)
{
    DWORD start_ticks = GetTickCount();

    // Init logging and crash dump support first
    InitLogging();
    CrashHandlerInit(g_hmodule);

    // Enable Data Execution Prevention
    if (!SetProcessDEPPolicy(PROCESS_DEP_ENABLE))
        WARN("SetProcessDEPPolicy failed (error %ld)", GetLastError());

    // Log system info
    LogSystemInfo();

    // Load config
    try {
        if (!g_game_config.load())
            ERR("Configuration has not been found in registry!");
    }
    catch (std::exception& e) {
        ERR("Failed to load configuration: %s", e.what());
    }

    // Log information from config
    INFO("Resolution: %dx%dx%d", g_game_config.res_width, g_game_config.res_height, g_game_config.res_bpp);
    INFO("Window Mode: %d", static_cast<int>(g_game_config.wnd_mode));
    INFO("Max FPS: %u", g_game_config.max_fps);
    INFO("Allow Overwriting Game Files: %d", static_cast<int>(g_game_config.allow_overwrite_game_files));

    /* Process messages in the same thread as DX processing (alternative: D3DCREATE_MULTITHREADED) */
    AsmWritter(0x00524C48, 0x00524C83).nop(); // disable msg loop thread
    AsmWritter(0x00524C48).call(0x00524E40);  // CreateMainWindow
    KeyGetFromFifo_hook.Install();

    /* General game hooks */
    InitGame_hook.Install();
    CleanupGame_hook.Install();
    RunGame_hook.Install();
    RenderInGame_hook.Install();
    AfterFullGameInit_hook.Install();
    DcUpdate_hook.Install();
    after_gs_init_hook.Install();

    RenderHitScreen_hook.Install();
    PlayerCreate_hook.Install();
    PlayerDestroy_hook.Install();

    /* Init modules */
    CommandsInit();
    GraphicsInit();
    InitGamma();
    GraphicsCaptureInit();
    NetworkInit();
    InitWndProc();
    InitHud();
    InitAutodownloader();
    InitScoreboard();
    InitKill();
    VfsApplyHooks(); /* Use new VFS without file count limit */
    SpectateModeInit();
    HighFpsInit();
    MiscInit();
    ServerInit();
    InputInit();
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
    ExperimentalInit();
#endif

    INFO("Installing hooks took %lu ms", GetTickCount() - start_ticks);

    return 1; /* success */
}

BOOL WINAPI DllMain(HINSTANCE instance_handle, [[maybe_unused]] DWORD fdw_reason, [[maybe_unused]] LPVOID lpv_reserved)
{
    g_hmodule = instance_handle;
    DisableThreadLibraryCalls(instance_handle);
    return TRUE;
}
