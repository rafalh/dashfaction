#include <windows.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../rf/os/os.h"
#include "../rf/multi.h"
#include "../rf/input.h"
#include "../rf/crt.h"
#include "../main/main.h"
#include "win32_console.h"
#include <xlog/xlog.h>

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
            // xlog::info("msg {}\n", msg.message);
        }

        if (win32_console_is_enabled()) {
            win32_console_poll_input();
        }
    },
};

LRESULT WINAPI wnd_proc(HWND wnd_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
    // xlog::trace("{:08x}: msg {} {:x} {:x}", GetTickCount(), get_win_msg_name(msg), w_param, l_param);
    if (rf::main_wnd && wnd_handle != rf::main_wnd) {
        xlog::warn("Got unknown window in the window procedure: hwnd {} msg {}", static_cast<void*>(wnd_handle), msg);
    }

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
            return 0; // DefWindowProcA(wnd_handle, msg, w_param, l_param);

        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY:
            rf::close_app_req = 1;
            break;

        case WM_PAINT:
            if (rf::is_dedicated_server)
                ++rf::console_redraw_counter;
            return DefWindowProcA(wnd_handle, msg, w_param, l_param);

        default:
            return DefWindowProcA(wnd_handle, msg, w_param, l_param);
    }

    return 0;
}

static FunHook<void(const char*, const char*, bool, bool)> os_init_window_server_hook{
    0x00524B70,
    [](const char* wclass, const char* title, bool hooks, bool server_console) {
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

static FunHook<void(char*, bool)> os_parse_params_hook{
    0x00523320,
    [](char* cmdline, bool skip_first) {
        std::string buf;
        bool quote = false;
        while (true) {
            char c = *cmdline;
            ++cmdline;

            if ((!quote && c == ' ') || c == '\0') {
                if (skip_first) {
                    skip_first = false;
                }
                else {
                    rf::CmdArg& cmd_arg = rf::cmdline_args[rf::cmdline_num_args++];
                    cmd_arg.arg = static_cast<char*>(rf::operator_new(buf.size() + 1));
                    std::strcpy(cmd_arg.arg, buf.c_str());
                    cmd_arg.is_done = false;
                }
                buf.clear();
                if (!c) {
                    break;
                }
            }
            else if (c == '"') {
                quote = !quote;
            }
            else {
                buf += c;
            }
        }
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

    // Fix quotes support in cmdline parsing
    os_parse_params_hook.install();

    // Apply patches from other files in 'os' dir
    void frametime_apply_patch();
    void timer_apply_patch();
    frametime_apply_patch();
    timer_apply_patch();

    win32_console_pre_init();
}
