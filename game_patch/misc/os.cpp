#include "../rf/os.h"
#include "../rf/multi.h"
#include "../rf/input.h"
#include <windows.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>

static LARGE_INTEGER g_qpc_frequency;

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
}
