#include "../rf/os.h"
#include "../rf/multi.h"
#include "../rf/input.h"
#include "../rf/graphics.h"
#include "../console/console.h"
#include "../main.h"
#include <windows.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>

static LARGE_INTEGER g_qpc_frequency;
static float g_frametime_history[1024];
static int g_frametime_history_index = 0;
static bool g_show_frametime_graph = false;

const char* get_win_msg_name(UINT msg);

FunHook<void()> os_poll_hook{
    0x00524B60,
    []() {
        // Note: When using dedicated server we get WM_PAINT messages all the time
        MSG msg;
        constexpr int limit = 4;
        for (int i = 0; i < limit; ++i) {
            if (!PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
                break;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            // xlog::info("msg %u\n", msg.message);
        }
    },
};

CodeInjection key_get_hook{
    0x0051F000,
    []() {
        // Process messages here because when watching videos main loop is not running
        rf::os_poll();
    },
};

LRESULT WINAPI wnd_proc(HWND wnd_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
    // xlog::trace("%08x: msg %s %x %x", GetTickCount(), get_win_msg_name(msg), w_param, l_param);

    for (unsigned i = 0; i < rf::num_msg_handlers; ++i) {
        rf::msg_handlers[i](msg, w_param, l_param);
    }

    switch (msg) {
    case WM_ACTIVATE:
        if (!rf::is_dedicated_server) {
            // Show cursor if window is not active
            if (w_param) {
                ShowCursor(FALSE);
                while (ShowCursor(FALSE) >= 0)
                    ;
            }
            else {
                ShowCursor(TRUE);
                while (ShowCursor(TRUE) < 0)
                    ;
            }
        }

        rf::is_main_wnd_active = w_param ? 1 : 0;
        return DefWindowProcA(wnd_handle, msg, w_param, l_param);

    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
        rf::close_app_req = 1;
        break;

    case WM_PAINT:
        if (rf::is_dedicated_server)
            ++rf::num_redraw_server;
        else
            return DefWindowProcA(wnd_handle, msg, w_param, l_param);
        break;

    default:
        return DefWindowProcA(wnd_handle, msg, w_param, l_param);
    }

    return 0;
}

void frametime_render()
{
    if (g_show_frametime_graph) {
        g_frametime_history[g_frametime_history_index] = rf::frametime;
        g_frametime_history_index = (g_frametime_history_index + 1) % std::size(g_frametime_history);
        float max_frametime = 0.0f;
        for (auto frametime : g_frametime_history) {
            max_frametime = std::max(max_frametime, frametime);
        }

        rf::gr_set_color_rgba(255, 255, 255, 128);
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

FunHook<int(int)> timer_get_hook{
    0x00504AB0,
    [](int scale) {
        static auto& timer_base = addr_as_ref<LARGE_INTEGER>(0x01751BF8);
        static auto& timer_last_value = addr_as_ref<LARGE_INTEGER>(0x01751BD0);
        // get QPC current value
        LARGE_INTEGER current_qpc_value;
        QueryPerformanceCounter(&current_qpc_value);
        // make sure time never goes backward
        if (current_qpc_value.QuadPart < timer_last_value.QuadPart) {
            current_qpc_value = timer_last_value;
        }
        timer_last_value = current_qpc_value;
        // Make sure we count from game start
        current_qpc_value.QuadPart -= timer_base.QuadPart;
        // Multiply with unit scale (eg. ms/us) before division
        current_qpc_value.QuadPart *= scale;
        // Divide by frequency using 64 bits and then cast to 32 bits
        // Note: sign of result does not matter because it is used only for deltas
        return static_cast<int>(current_qpc_value.QuadPart / g_qpc_frequency.QuadPart);
    },
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

void os_do_patch()
{
    // Process messages in the same thread as DX processing (alternative: D3DCREATE_MULTITHREADED)
    AsmWriter(0x00524C48, 0x00524C83).nop(); // disable msg loop thread
    AsmWriter(0x00524C48).call(0x00524E40);  // CreateMainWindow
    key_get_hook.install();
    os_poll_hook.install();

    // Remove Sleep calls in timer_init
    AsmWriter(0x00504A67, 0x00504A82).nop();

    // Subclass window
    write_mem_ptr(0x00524E66, &wnd_proc);

    // Fix timer_get handling of frequency greater than 2MHz (sign bit is set in 32 bit dword)
    QueryPerformanceFrequency(&g_qpc_frequency);
    timer_get_hook.install();

    // Fix incorrect frame time calculation
    AsmWriter(0x00509595).nop(2);
    write_mem<u8>(0x00509532, asm_opcodes::jmp_rel_short);
    frametime_calculate_sleep_hook.install();

    // Disable keyboard hooks (they were supposed to block alt-tab; they does not work in modern OSes anyway)
    write_mem<u8>(0x00524C98, asm_opcodes::jmp_rel_short);

    // Set initial FPS limit
    write_mem<float>(0x005094CA, 1.0f / g_game_config.max_fps);

    // Commands
    max_fps_cmd.register_cmd();
    frametime_graph_cmd.register_cmd();
}
