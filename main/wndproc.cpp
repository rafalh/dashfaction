#include "stdafx.h"
#include "wndproc.h"
#include "rf.h"
#include "utils.h"

const char* GetWndMsgName(UINT uMsg);

static bool IsMouseEnabled()
{
    return rf::g_bMouseInitialized && rf::g_bIsActive;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //TRACE("%08x: msg %s %x %x", GetTickCount(), GetWndMsgName(uMsg), wParam, lParam);

    for (unsigned i = 0; i < rf::g_cMsgHandlers; ++i)
        rf::g_MsgHandlers[i](uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_ACTIVATE:
        if (!rf::g_bDedicatedServer) {
            // Show cursor if window is not active
            if (wParam) {
                ShowCursor(FALSE);
                while (ShowCursor(FALSE) >= 0);
            } else {
                ShowCursor(TRUE);
                while (ShowCursor(TRUE) < 0);
            }
        }

        rf::g_bIsActive = wParam ? 1 : 0;
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
        rf::g_bClose = 1;
        break;

    case WM_PAINT:
        if (rf::g_bDedicatedServer)
            ++rf::g_cRedrawServer;
        else
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        break;

    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
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
