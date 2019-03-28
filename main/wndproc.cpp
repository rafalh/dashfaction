#include "stdafx.h"
#include "wndproc.h"
#include "rf.h"
#include "utils.h"

const char* GetWndMsgName(UINT Msg);

static bool IsMouseEnabled()
{
    return rf::g_MouseInitialized && rf::g_IsActive;
}

LRESULT WINAPI WndProc(HWND Wnd_handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    //TRACE("%08x: msg %s %x %x", GetTickCount(), GetWndMsgName(Msg), wParam, lParam);

    for (unsigned i = 0; i < rf::g_cMsgHandlers; ++i)
        rf::g_MsgHandlers[i](Msg, wParam, lParam);

    switch (Msg) {
    case WM_ACTIVATE:
        if (!rf::g_IsDedicatedServer) {
            // Show cursor if window is not active
            if (wParam) {
                ShowCursor(FALSE);
                while (ShowCursor(FALSE) >= 0);
            } else {
                ShowCursor(TRUE);
                while (ShowCursor(TRUE) < 0);
            }
        }

        rf::g_IsActive = wParam ? 1 : 0;
        return DefWindowProcA(Wnd_handle, Msg, wParam, lParam);

    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
        rf::g_Close = 1;
        break;

    case WM_PAINT:
        if (rf::g_IsDedicatedServer)
            ++rf::g_cRedrawServer;
        else
            return DefWindowProcA(Wnd_handle, Msg, wParam, lParam);
        break;

    default:
        return DefWindowProcA(Wnd_handle, Msg, wParam, lParam);
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
