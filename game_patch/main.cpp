#include <common/GameConfig.h>
#include "main.h"
#include "stdafx.h"
#include "level_autodl/autodl.h"
#include "console/console.h"
#include "crash_handler_stub.h"
#include "debug/debug.h"
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
#include "utils/os-utils.h"
#include "utils/list-utils.h"
#include "server/server.h"
#include "input/input.h"
#include "rf/network.h"
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <common/version.h>

#include <xlog/ConsoleAppender.h>
#include <xlog/FileAppender.h>
#include <xlog/Win32Appender.h>
#include <xlog/xlog.h>

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
        // xlog::info("msg %u\n", msg.message);
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
        xlog::info("Initializing game...");
        InitGame_hook.CallTarget();
        PackfileDisableOverriding();
        xlog::info("Game initialized (%lu ms).", GetTickCount() - start_ticks);
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
        DebugInit();

        xlog::info("Game fully initialized");
        xlog::LoggerConfig::get().flush_appenders();
    },
};

CodeInjection cleanup_game_hook{
    0x004B2821,
    []() {
        ResetGammaRamp();
        ServerCleanup();
        DebugCleanup();
    },
};

CodeInjection before_frame_hook{
    0x004B2818,
    []() {
        ProcessWaitingMessages();
        HighFpsUpdate();
        ServerDoFrame();
        DebugDoUpdate();
    },
};

CodeInjection after_level_render_hook{
    0x00432375,
    []() {
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRenderInGame();
#endif
        DebugRender();
    },
};

CodeInjection after_frame_render_hook{
    0x004B2E3F,
    []() {
        // Draw on top (after scene)

        if (rf::is_multi)
            SpectateModeDrawUI();

        GraphicsDrawFpsCounter();
        RenderDownloadProgress();
#if !defined(NDEBUG) && defined(HAS_EXPERIMENTAL)
        ExperimentalRender();
#endif
        DebugRenderUI();
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
        xlog::info("Loading level: %s", level_filename.CStr());
        if (save_filename.Size() > 0)
            xlog::info("Restoring game from save file: %s", save_filename.CStr());
        int ret = RflLoad_hook.CallTarget(level_filename, save_filename, error);
        if (ret != 0)
            xlog::warn("Loading failed: %s", error);
        else {
            HighFpsAfterLevelLoad(level_filename);
            MiscAfterLevelLoad(level_filename);
        }
        return ret;
    },
};

FunHook<void(bool)> GameWideOnLevelStart_hook{
    0x00435DF0,
    [](bool is_auto_level_load) {
        GameWideOnLevelStart_hook.CallTarget(is_auto_level_load);
        xlog::info("Level loaded: %s%s", rf::level_filename.CStr(), is_auto_level_load ? " (caused by event)" : "");
    },
};

class RfConsoleLogAppender : public xlog::Appender
{
    std::vector<std::pair<std::string, xlog::Level>> m_startup_buf;

public:
    RfConsoleLogAppender()
    {
#ifdef NDEBUG
        set_level(xlog::Level::warn);
#endif
    }

protected:
    virtual void append([[maybe_unused]] xlog::Level level, const std::string& str) override
    {
        auto& console_inited = AddrAsRef<bool>(0x01775680);
        if (console_inited) {
            flush_startup_buf();

            uint32_t color = color_from_level(level);
            rf::DcPrint(str.c_str(), &color);
        }
        else {
            m_startup_buf.push_back({str, level});
        }
    }

    virtual void flush() override
    {
        auto& console_inited = AddrAsRef<bool>(0x01775680);
        if (console_inited) {
            flush_startup_buf();
        }
    }

private:
    uint32_t color_from_level(xlog::Level level) const
    {
        switch (level) {
            case xlog::Level::error:
                return 0xFF0000FF;
            case xlog::Level::warn:
                return 0xFF00FFFF;
            case xlog::Level::info:
                return 0xFFAAAAAA;
            default:
                return 0xFF888888;
        }
    }

    void flush_startup_buf()
    {
        for (auto& p : m_startup_buf) {
            uint32_t color = color_from_level(p.second);
            rf::DcPrint(p.first.c_str(), &color);
        }
        m_startup_buf.clear();
    }
};

void InitLogging()
{
    CreateDirectoryA("logs", nullptr);
    xlog::LoggerConfig::get()
        .add_appender<xlog::FileAppender>("logs/DashFaction.log", false)
        .add_appender<xlog::ConsoleAppender>()
        .add_appender<xlog::Win32Appender>()
        .add_appender<RfConsoleLogAppender>();
    xlog::info("Dash Faction %s (%s %s)", VERSION_STR, __DATE__, __TIME__);
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
        xlog::info() << "Real system version: " << getRealOsVersion();
        xlog::info() << "Emulated system version: " << getOsVersion();
        auto wine_ver = GetWineVersion();
        if (wine_ver)
            xlog::info() << "Running on Wine: " << wine_ver.value();

        xlog::info("Running as %s (elevation type: %s)", IsCurrentUserAdmin() ? "admin" : "user", GetProcessElevationType());
        xlog::info() << "CPU Brand: " << getCpuBrand();
        xlog::info() << "CPU ID: " << getCpuId();
        LARGE_INTEGER qpc_freq;
        QueryPerformanceFrequency(&qpc_freq);
        xlog::info("QPC Frequency: %08lX %08lX", static_cast<DWORD>(qpc_freq.HighPart), qpc_freq.LowPart);
    }
    catch (std::exception& e) {
        xlog::error("Failed to read system info: %s", e.what());
    }
}

extern "C" void subhook_unk_opcode_handler(uint8_t* opcode)
{
    xlog::error("SubHook unknown opcode 0x%X at 0x%p", *opcode, opcode);
}

extern "C" DWORD DF_DLL_EXPORT Init([[maybe_unused]] void* unused)
{
    DWORD start_ticks = GetTickCount();

    // Init logging and crash dump support first
    InitLogging();
    CrashHandlerStubInstall(g_hmodule);

    // Enable Data Execution Prevention
    if (!SetProcessDEPPolicy(PROCESS_DEP_ENABLE))
        xlog::warn("SetProcessDEPPolicy failed (error %ld)", GetLastError());

    // Log system info
    LogSystemInfo();

    // Load config
    try {
        if (!g_game_config.load())
            xlog::error("Configuration has not been found in registry!");
    }
    catch (std::exception& e) {
        xlog::error("Failed to load configuration: %s", e.what());
    }

    // Log information from config
    xlog::info("Resolution: %dx%dx%d", g_game_config.res_width, g_game_config.res_height, g_game_config.res_bpp);
    xlog::info("Window Mode: %d", static_cast<int>(g_game_config.wnd_mode));
    xlog::info("Max FPS: %u", g_game_config.max_fps);
    xlog::info("Allow Overwriting Game Files: %d", static_cast<int>(g_game_config.allow_overwrite_game_files));

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
    ApplyHudPatches();
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
    DebugApplyPatches();

    xlog::info("Installing hooks took %lu ms", GetTickCount() - start_ticks);

    return 1; // success
}

BOOL WINAPI DllMain(HINSTANCE instance_handle, [[maybe_unused]] DWORD fdw_reason, [[maybe_unused]] LPVOID lpv_reserved)
{
    g_hmodule = instance_handle;
    DisableThreadLibraryCalls(instance_handle);
    return TRUE;
}
