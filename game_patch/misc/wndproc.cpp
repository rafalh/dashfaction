#include "wndproc.h"
#include "../rf/misc.h"
#include "../rf/network.h"
#include "../rf/input.h"
#include <windows.h>
#include <patch_common/AsmWriter.h>

const char* GetWndMsgName(UINT msg);

static bool IsMouseEnabled()
{
    return rf::mouse_initialized && rf::is_main_wnd_active;
}

LRESULT WINAPI WndProc(HWND wnd_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
    // TRACE("%08x: msg %s %x %x", GetTickCount(), GetWndMsgName(msg), w_param, l_param);

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

void InitWndProc()
{
    // Subclass window
    WriteMemPtr(0x00524E66, &WndProc);

    // Disable mouse when window is not active
    AsmWriter(0x0051DC70).call(IsMouseEnabled);
}
