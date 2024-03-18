#include "../rf/os/frametime.h"
#include "../hud/hud.h"
#include "../main/main.h"
#include "../rf/gameseq.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/hud.h"
#include "../rf/level.h"
#include "../rf/multi.h"
#include "console.h"
#include <common/version/version.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>

static float g_frametime_history[1024];
static int g_frametime_history_index = 0;
static bool g_show_frametime_graph = false;

static void frametime_render_graph()
{
    if (g_show_frametime_graph) {
        g_frametime_history[g_frametime_history_index] = rf::frametime;
        g_frametime_history_index = (g_frametime_history_index + 1) % std::size(g_frametime_history);
        float max_frametime = 0.0f;
        for (auto frametime : g_frametime_history) {
            max_frametime = std::max(max_frametime, frametime);
        }

        rf::gr::set_color(255, 255, 255, 128);
        int scr_w = rf::gr::screen_width();
        int scr_h = rf::gr::screen_height();
        for (unsigned i = 0; i < std::size(g_frametime_history); ++i) {
            int slot_index = (g_frametime_history_index + 1 + i) % std::size(g_frametime_history);
            int x = scr_w - i - 1;
            int h = static_cast<int>(g_frametime_history[slot_index] / max_frametime * 100.0f);
            rf::gr::rect(x, scr_h - h, 1, h);
        }
    }
}

static void frametime_render_fps_counter()
{
    if (g_game_config.fps_counter && !rf::hud_disabled) {
        auto text = string_format("FPS: %.1f", rf::current_fps);
        rf::gr::set_color(0, 255, 0, 255);
        int x = rf::gr::screen_width() - (g_game_config.big_hud ? 165 : 90);
        int y = 10;
        if (rf::gameseq_in_gameplay()) {
            y = g_game_config.big_hud ? 110 : 60;
            if (hud_weapons_is_double_ammo()) {
                y += g_game_config.big_hud ? 80 : 40;
            }
        }

        int font_id = hud_get_default_font();
        rf::gr::string(x, y, text.c_str(), font_id);
    }
}

static void frametime_render_bomb_derandomized()
{
    // This only actually needs to be displayed on L20S3 but you can't access
    // the level.filename at this point in code (as this is drawn even when no
    // level loaded e.g. on load screens etc.), and having it always showing helps
    // prevent splicing for speedruns etc
    if (g_game_config.derandomize_bomb) {
        auto text = string_format("No Bomb RNG", "null");
        rf::gr::set_color(0, 255, 0, 255);
        int x = rf::gr::screen_width() - (g_game_config.big_hud ? 180 : 105);
        int y = g_game_config.fps_counter ? 20 : 10;
        if (rf::gameseq_in_gameplay()) {
            y = g_game_config.big_hud ? 120 : 70;
            if (hud_weapons_is_double_ammo()) {
                y += g_game_config.big_hud ? 80 : 40;
            }
        }

        int font_id = hud_get_default_font();
        rf::gr::string(x, y, text.c_str(), font_id);
    }
}

void frametime_render_ui()
{
    frametime_render_fps_counter();
    frametime_render_graph();
    frametime_render_bomb_derandomized();
}


ConsoleCommand2 fps_counter_cmd{
    "fps_counter",
    []() {
        g_game_config.fps_counter = !g_game_config.fps_counter;
        g_game_config.save();
        rf::console::printf("FPS counter display is %s", g_game_config.fps_counter ? "enabled" : "disabled");
    },
    "Toggle FPS counter",
    "fps_counter",
};

CallHook<void(int)> frametime_calculate_sleep_hook{
    0x005095B4,
    [](int ms) {
        --ms;
        if (ms > 0) {
            frametime_calculate_sleep_hook.call_target(ms);
        }
    },
};

FunHook<void()> frametime_reset_hook{
    0x00509490,
    []() {
        frametime_reset_hook.call_target();

        // Set initial FPS limit
        unsigned max_fps =
            rf::is_dedicated_server ? g_game_config.server_max_fps.value() : g_game_config.max_fps.value();
        rf::frametime_min = 1.0f / static_cast<float>(max_fps);
    },
};

ConsoleCommand2 max_fps_cmd{
    "maxfps",
    [](std::optional<int> limit_opt) {
        if (limit_opt) {
            int new_limit = limit_opt.value();
#if VERSION_TYPE != VERSION_TYPE_DEV
            new_limit = std::clamp<int>(new_limit, GameConfig::min_fps_limit, GameConfig::max_fps_limit);
#endif
            if (rf::is_dedicated_server) {
                g_game_config.server_max_fps = new_limit;
            }
            else {
                g_game_config.max_fps = new_limit;
            }
            g_game_config.save();
            rf::frametime_min = 1.0f / new_limit;
        }
        else
            rf::console::printf("Maximal FPS: %.1f", 1.0f / rf::frametime_min);
    },
    "Sets maximal FPS",
    "maxfps <limit>",
};

ConsoleCommand2 frametime_graph_cmd{
    "frametime_graph",
    []() { g_show_frametime_graph = !g_show_frametime_graph; },
};

void frametime_apply_patch()
{
    // Fix incorrect frame time calculation
    AsmWriter(0x00509595).nop(2);
    write_mem<u8>(0x00509532, asm_opcodes::jmp_rel_short);
    frametime_calculate_sleep_hook.install();

    // Set initial FPS limit
    frametime_reset_hook.install();

    // Commands
    max_fps_cmd.register_cmd();
    frametime_graph_cmd.register_cmd();
    fps_counter_cmd.register_cmd();
}
