#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>
#include "console.h"
#include "../rf/gr.h"
#include "../main.h"

static float g_frametime_history[1024];
static int g_frametime_history_index = 0;
static bool g_show_frametime_graph = false;

void frametime_render()
{
    if (g_show_frametime_graph) {
        g_frametime_history[g_frametime_history_index] = rf::frametime;
        g_frametime_history_index = (g_frametime_history_index + 1) % std::size(g_frametime_history);
        float max_frametime = 0.0f;
        for (auto frametime : g_frametime_history) {
            max_frametime = std::max(max_frametime, frametime);
        }

        rf::gr_set_color(255, 255, 255, 128);
        int scr_w = rf::gr_screen_width();
        int scr_h = rf::gr_screen_height();
        for (unsigned i = 0; i < std::size(g_frametime_history); ++i) {
            int slot_index = (g_frametime_history_index + 1 + i) % std::size(g_frametime_history);
            int x = scr_w - i - 1;
            int h = g_frametime_history[slot_index] / max_frametime * 100.0f;
            rf::gr_rect(x, scr_h - h, 1, h);
        }
    }
}

CallHook<void(int)> frametime_calculate_sleep_hook{
    0x005095B4,
    [](int ms) {
        --ms;
        if (ms > 0) {
            frametime_calculate_sleep_hook.call_target(ms);
        }
    },
};

ConsoleCommand2 max_fps_cmd{
    "maxfps",
    [](std::optional<int> limit_opt) {
        if (limit_opt) {
#ifdef NDEBUG
            int new_limit = std::clamp<int>(limit_opt.value(), MIN_FPS_LIMIT, MAX_FPS_LIMIT);
#else
            int new_limit = limit_opt.value();
#endif
            g_game_config.max_fps = new_limit;
            g_game_config.save();
            rf::framerate_min = 1.0f / new_limit;
        }
        else
            rf::console_printf("Maximal FPS: %.1f", 1.0f / rf::framerate_min);
    },
    "Sets maximal FPS",
    "maxfps <limit>",
};

ConsoleCommand2 frametime_graph_cmd{
    "frametime_graph",
    []() {
        g_show_frametime_graph = !g_show_frametime_graph;
    },
};

void frametime_apply_patch()
{
    // Fix incorrect frame time calculation
    AsmWriter(0x00509595).nop(2);
    write_mem<u8>(0x00509532, asm_opcodes::jmp_rel_short);
    frametime_calculate_sleep_hook.install();

    // Set initial FPS limit
    write_mem<float>(0x005094CA, 1.0f / g_game_config.max_fps);

    // Commands
    max_fps_cmd.register_cmd();
    frametime_graph_cmd.register_cmd();
}
