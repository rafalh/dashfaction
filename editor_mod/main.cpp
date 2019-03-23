#include "version.h"
#include "exports.h"
#include <MemUtils.h>
#include <AsmOpcodes.h>
#include <AsmWritter.h>
#include <cstdio>

#define LAUNCHER_FILENAME "DashFactionLauncher.exe"

HMODULE g_hModule;
HWND g_hEditorWnd;
WNDPROC g_pEditorWndProc_Orig;

static const auto g_pEditorApp = (void*)0x006F9DA0;

void OpenLevel(const char *pszPath)
{
    void *pDocManager = *(void**)(((BYTE*)g_pEditorApp) + 0x80);
    void *pDocManager_Vtbl = *(void**)pDocManager;
    typedef int(__thiscall *CDocManager_OpenDocumentFile_Ptr)(void *This, LPCSTR lpString2);
    CDocManager_OpenDocumentFile_Ptr pDocManager_OpenDocumentFile = (CDocManager_OpenDocumentFile_Ptr)*(((void**)pDocManager_Vtbl) + 7);
    pDocManager_OpenDocumentFile(pDocManager, pszPath);
}

LRESULT CALLBACK EditorWndProc_New(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DROPFILES)
    {
        HDROP hDropInfo = (HDROP)wParam;
        char sItem[MAX_PATH];
        // Handle only first droped file
        if (DragQueryFile(hDropInfo, 0, sItem, sizeof(sItem)))
            OpenLevel(sItem);
        DragFinish(hDropInfo);
    }
    // Call original procedure
    return g_pEditorWndProc_Orig(hwnd, uMsg, wParam, lParam);
}

BOOL CEditorApp__InitInstance_AfterHook()
{
    g_hEditorWnd = GetActiveWindow();
    g_pEditorWndProc_Orig = (WNDPROC)GetWindowLongPtr(g_hEditorWnd, GWLP_WNDPROC);
    SetWindowLongPtr(g_hEditorWnd, GWLP_WNDPROC, (LONG)EditorWndProc_New);
    DWORD ExStyle = GetWindowLongPtr(g_hEditorWnd, GWL_EXSTYLE);
    SetWindowLongPtr(g_hEditorWnd, GWL_EXSTYLE, ExStyle | WS_EX_ACCEPTFILES);
    return TRUE;
}

extern "C" DWORD DLL_EXPORT Init(void *pUnused)
{
    // Prepare command
    static char CmdBuf[512];
    GetModuleFileNameA(g_hModule, CmdBuf, sizeof(CmdBuf));
    char *Ptr = strrchr(CmdBuf, '\\');
    strcpy(Ptr, "\\" LAUNCHER_FILENAME " -level \"");

    // Change command for starting level test
    WriteMemPtr(0x00447973 + 1, CmdBuf);
    WriteMemPtr(0x00447CB9 + 1, CmdBuf);

    // Zero first argument for CreateProcess call
    AsmWritter(0x00447B32, 0x00447B39).nop();
    AsmWritter(0x00447B32).xor_(AsmRegs::eax, AsmRegs::eax);
    AsmWritter(0x00448024, 0x0044802B).nop();
    AsmWritter(0x00448024).xor_(AsmRegs::eax, AsmRegs::eax);

    // InitInstance hook
    AsmWritter(0x00482C84).call(CEditorApp__InitInstance_AfterHook);

    return 1; // success
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
    g_hModule = (HMODULE)hInstance;
    DisableThreadLibraryCalls(hInstance);
    return TRUE;
}
