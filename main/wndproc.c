#include "wndproc.h"
#include "rf.h"
#include "utils.h"
#include <windows.h>

static BOOL IsMouseEnabled(void)
{
    return *g_pbMouseInitialized && *g_pbIsActive;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    unsigned i;
    
    for(i = 0; i < *g_pcMsgHandlers; ++i)
        g_MsgHandlers[i](uMsg, wParam, lParam);
    
    switch(uMsg)
    {
        case WM_ACTIVATE:
            *g_pbIsActive = wParam ? 1 : 0;
            break;
        
        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY:
            *g_pbClose = 1;
            break;
        
        case WM_PAINT:
            if(*g_pbDedicatedServer)
                ++(*g_pcRedrawServer);
            else
                return DefWindowProcA(hWnd, uMsg, wParam, lParam);
            break;
        
        default:
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    
    return 0;
}

void InitWndProc(void)
{
    // Subclass window
	WriteMemPtr((PVOID)0x00524E66, WndProc);
    
    // Disable mouse when window is not active
    WriteMemUInt8((PVOID)0x0051DC70, ASM_LONG_CALL_REL);
    WriteMemPtr((PVOID)(0x0051DC70 + 1), (PVOID)((ULONG_PTR)IsMouseEnabled - (0x0051DC70 + 0x5)));
}
