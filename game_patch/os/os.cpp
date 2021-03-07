#include <windows.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include "../rf/os/os.h"
#include "../rf/multi.h"
#include "../rf/input.h"
#include "../main/main.h"
#include "win32_console.h"

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

        if (win32_console_is_enabled()) {
            win32_console_poll_input();
        }
    },
};

LRESULT WINAPI wnd_proc(HWND wnd_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
    // xlog::trace("%08x: msg %s %x %x", GetTickCount(), get_win_msg_name(msg), w_param, l_param);

    for (int i = 0; i < rf::num_msg_handlers; ++i) {
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

        rf::is_main_wnd_active = w_param;
        return DefWindowProcA(wnd_handle, msg, w_param, l_param);

    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
        rf::close_app_req = true;
        break;

    case WM_PAINT:
        if (rf::is_dedicated_server)
            ++rf::console_redraw_counter;
        else
            return DefWindowProcA(wnd_handle, msg, w_param, l_param);
        break;

    default:
        return DefWindowProcA(wnd_handle, msg, w_param, l_param);
    }

    return 0;
}

static FunHook<void(const char *, const char *, bool, bool)> os_init_window_server_hook{
    0x00524B70,
    [](const char *wclass, const char *title, bool hooks, bool server_console) {
        if (server_console) {
            win32_console_init();
        }
        if (!win32_console_is_enabled()) {
            os_init_window_server_hook.call_target(wclass, title, hooks, server_console);
        }
    },
};

static FunHook<void()> os_close_hook{
    0x00525240,
    []() {
        os_close_hook.call_target();
        win32_console_close();
    },
};

void os_apply_patch()
{
    // Process messages in the same thread as DX processing (alternative: D3DCREATE_MULTITHREADED)
    AsmWriter(0x00524C48, 0x00524C83).nop(); // disable msg loop thread
    AsmWriter(0x00524C48).call(0x00524E40);  // os_create_main_window
    os_poll_hook.install();

    // Subclass window
    write_mem_ptr(0x00524E66, &wnd_proc);

    // Disable keyboard hooks (they were supposed to block alt-tab; they does not work in modern OSes anyway)
    write_mem<u8>(0x00524C98, asm_opcodes::jmp_rel_short);

    // Hooks for win32 console support
    os_init_window_server_hook.install();
    os_close_hook.install();

    // Apply patches from other files in 'os' dir
    void frametime_apply_patch();
    void timer_apply_patch();
    frametime_apply_patch();
    timer_apply_patch();

    win32_console_pre_init();
}
