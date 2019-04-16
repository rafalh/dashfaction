#include "wndproc.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"

const char* GetWndMsgName(UINT msg);

static bool IsMouseEnabled()
{
    return rf::g_MouseInitialized && rf::g_IsActive;
}

LRESULT WINAPI WndProc(HWND wnd_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
    // TRACE("%08x: msg %s %x %x", GetTickCount(), GetWndMsgName(msg), w_param, l_param);

    for (unsigned i = 0; i < rf::g_NumMsgHandlers; ++i) {
        rf::g_MsgHandlers[i](msg, w_param, l_param);
    }

    switch (msg) {
    case WM_ACTIVATE:
        if (!rf::g_IsDedicatedServer) {
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

        rf::g_IsActive = w_param ? 1 : 0;
        return DefWindowProcA(wnd_handle, msg, w_param, l_param);

    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
        rf::g_Close = 1;
        break;

    case WM_PAINT:
        if (rf::g_IsDedicatedServer)
            ++rf::g_NumRedrawServer;
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
    AsmWritter(0x0051DC70).call(IsMouseEnabled);
}
