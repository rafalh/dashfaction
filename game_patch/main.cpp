#include <common/GameConfig.h>
#include "main.h"
#include "rf.h"
#include "stdafx.h"
#include "level_autodl/autodl.h"
#include "console/console.h"
#include "debug/crashhandler.h"
#include "debug/debug_cmd.h"
#include "exports.h"
#include "graphics/gamma.h"
#include "graphics/graphics.h"
#include "graphics/capture.h"
#include "in_game_ui/hud.h"
#include "in_game_ui/scoreboard.h"
#include "in_game_ui/spectate_mode.h"
#include "multi/kill.h"
#include "multi/network.h"
#include "misc/misc.h"
#include "misc/packfile.h"
#include "misc/wndproc.h"
#include "misc/high_fps.h"
#include "utils/utils.h"
#include "server/server.h"
#include "input/input.h"
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <common/version.h>

#include <log/ConsoleAppender.h>
#include <log/FileAppender.h>
#include <log/Win32Appender.h>

#ifdef HAS_EXPERIMENTAL
#include "misc/experimental.h"
#endif

GameConfig g_game_config;
HMODULE g_hmodule;
std::unordered_map<rf::Player*, PlayerAdditionalData> g_player_additional_data_map;

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
    auto player_list = SinglyLinkedList{rf::player_list};
    for (auto& player : player_list) {
        if (query(player.name))
            consumer(&player);
    }
}

PlayerAdditionalData& GetPlayerAdditionalData(rf::Player* player)
{
    return g_player_additional_data_map[player];
}

CallHook<void()> InitGame_hook{
    0x004B27CD,
    []() {
        auto start_ticks = GetTickCount();
        INFO("Initializing game...");
        InitGame_hook.CallTarget();
        INFO("Game initialized (%lu ms).", GetTickCount() - start_ticks);
    },
};

CodeInjection after_full_game_init_hook{
    0x004B26C6,
    []() {
        SpectateModeAfterFullGameInit();
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalInitAfterGame();
#endif
        GraphicsCaptureAfterGameInit();
        ConsoleInit();
        MiscAfterFullGameInit();

        INFO("Game fully initialized");
    },
};

CodeInjection cleanup_game_hook{
    0x004B2821,
    []() {
        ResetGammaRamp();
        ServerCleanup();
    },
};

CodeInjection before_frame_hook{
    0x004B2818,
    []() {
        ProcessWaitingMessages();
        HighFpsUpdate();
        ServerDoFrame();
    },
};

CodeInjection after_level_render_hook{
    0x00432375,
    []() {
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRenderInGame();
#endif
        DebugRender3d();
    },
};

CodeInjection after_frame_render_hook{
    0x004B2E3F,
    []() {
        // Draw on top (after scene)

        if (rf::is_net_game)
            SpectateModeDrawUI();

        GraphicsDrawFpsCounter();
        RenderDownloadProgress();
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRender();
#endif
        DebugRender2d();
    },
};

CodeInjection KeyGetFromFifo_hook{
    0x0051F000,
    []() {
        // Process messages here because when watching videos main loop is not running
        ProcessWaitingMessages();
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
        g_player_additional_data_map.erase(player);
    },
};

FunHook<int(rf::String&, rf::String&, char*)> RflLoad_hook{
    0x0045C540,
    [](rf::String& level_filename, rf::String& save_filename, char* error) {
        INFO("Loading level: %s", level_filename.CStr());
        if (save_filename.Size() > 0)
            INFO("Restoring game from save file: %s", save_filename.CStr());
        int ret = RflLoad_hook.CallTarget(level_filename, save_filename, error);
        if (ret != 0)
            WARN("Loading failed: %s", error);
        else
            HighFpsAfterLevelLoad(level_filename);
        return ret;
    },
};

FunHook<void(bool)> GameWideOnLevelStart_hook{
    0x00435DF0,
    [](bool is_auto_level_load) {
        GameWideOnLevelStart_hook.CallTarget(is_auto_level_load);
        INFO("Level loaded: %s%s", rf::level_filename.CStr(), is_auto_level_load ? " (caused by event)" : "");
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
    INFO("Dash Faction %s (%s %s)", VERSION_STR, __DATE__, __TIME__);
}

std::optional<std::string> GetWineVersion()
{
    auto ntdll_handle = GetModuleHandleA("ntdll.dll");
    auto wine_get_version = reinterpret_cast<const char*(*)()>(GetProcAddress(ntdll_handle, "wine_get_version"));
    if (!wine_get_version)
        return {};
    auto ver = wine_get_version();
    return {ver};
}

void LogSystemInfo()
{
    try {
        logging::Logger::root().info() << "Real system version: " << getRealOsVersion();
        logging::Logger::root().info() << "Emulated system version: " << getOsVersion();
        auto wine_ver = GetWineVersion();
        if (wine_ver)
            logging::Logger::root().info() << "Running on Wine: " << wine_ver.value();

        INFO("Running as %s (elevation type: %s)", IsUserAdmin() ? "admin" : "user", GetProcessElevationType());
        logging::Logger::root().info() << "CPU Brand: " << getCpuBrand();
        logging::Logger::root().info() << "CPU ID: " << getCpuId();
        LARGE_INTEGER qpc_freq;
        QueryPerformanceFrequency(&qpc_freq);
        INFO("QPC Frequency: %08lX %08lX", static_cast<DWORD>(qpc_freq.HighPart), qpc_freq.LowPart);
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

    // Process messages in the same thread as DX processing (alternative: D3DCREATE_MULTITHREADED)
    AsmWriter(0x00524C48, 0x00524C83).nop(); // disable msg loop thread
    AsmWriter(0x00524C48).call(0x00524E40);  // CreateMainWindow
    KeyGetFromFifo_hook.Install();

    // General game hooks
    InitGame_hook.Install();
    after_full_game_init_hook.Install();
    cleanup_game_hook.Install();
    before_frame_hook.Install();
    after_level_render_hook.Install();
    after_frame_render_hook.Install();
    PlayerCreate_hook.Install();
    PlayerDestroy_hook.Install();
    RflLoad_hook.Install();
    GameWideOnLevelStart_hook.Install();

    // Init modules
    ConsoleApplyPatches();
    GraphicsInit();
    InitGamma();
    GraphicsCaptureInit();
    NetworkInit();
    InitWndProc();
    InitHud();
    InitAutodownloader();
    InitScoreboard();
    InitKill();
    PackfileApplyPatches();
    SpectateModeInit();
    HighFpsInit();
    MiscInit();
    ServerInit();
    InputInit();
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
    ExperimentalInit();
#endif
    DebugCmdApplyPatches();
    DebugCmdInit();

    INFO("Installing hooks took %lu ms", GetTickCount() - start_ticks);

    return 1; // success
}

BOOL WINAPI DllMain(HINSTANCE instance_handle, [[maybe_unused]] DWORD fdw_reason, [[maybe_unused]] LPVOID lpv_reserved)
{
    g_hmodule = instance_handle;
    DisableThreadLibraryCalls(instance_handle);
    return TRUE;
}
