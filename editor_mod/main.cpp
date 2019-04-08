#include "exports.h"
#include "version.h"
#include <AsmOpcodes.h>
#include <AsmWritter.h>
#include <MemUtils.h>
#include <cstdio>

#define LAUNCHER_FILENAME "DashFactionLauncher.exe"

HMODULE g_Module;
HWND g_EditorWnd;
WNDPROC g_EditorWndProc_Orig;

static const auto g_EditorApp = (void*)0x006F9DA0;

void OpenLevel(const char* Path)
{
    void* DocManager = *(void**)(((BYTE*)g_EditorApp) + 0x80);
    void* DocManager_Vtbl = *(void**)DocManager;
    typedef int(__thiscall * CDocManager_OpenDocumentFile_Ptr)(void* This, LPCSTR String2);
    CDocManager_OpenDocumentFile_Ptr pDocManager_OpenDocumentFile =
        (CDocManager_OpenDocumentFile_Ptr) * (((void**)DocManager_Vtbl) + 7);
    pDocManager_OpenDocumentFile(DocManager, Path);
}

LRESULT CALLBACK EditorWndProc_New(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DROPFILES) {
        HDROP Drop = (HDROP)wParam;
        char FileName[MAX_PATH];
        // Handle only first droped file
        if (DragQueryFile(Drop, 0, FileName, sizeof(FileName)))
            OpenLevel(FileName);
        DragFinish(Drop);
    }
    // Call original procedure
    return g_EditorWndProc_Orig(hwnd, uMsg, wParam, lParam);
}

BOOL CEditorApp__InitInstance_AfterHook()
{
    g_EditorWnd = GetActiveWindow();
    g_EditorWndProc_Orig = (WNDPROC)GetWindowLongPtr(g_EditorWnd, GWLP_WNDPROC);
    SetWindowLongPtr(g_EditorWnd, GWLP_WNDPROC, (LONG)EditorWndProc_New);
    DWORD ExStyle = GetWindowLongPtr(g_EditorWnd, GWL_EXSTYLE);
    SetWindowLongPtr(g_EditorWnd, GWL_EXSTYLE, ExStyle | WS_EX_ACCEPTFILES);
    return TRUE;
}

extern "C" DWORD DLL_EXPORT Init([[maybe_unused]] void* Unused)
{
    // Prepare command
    static char CmdBuf[512];
    GetModuleFileNameA(g_Module, CmdBuf, sizeof(CmdBuf));
    char* Ptr = strrchr(CmdBuf, '\\');
    strcpy(Ptr, "\\" LAUNCHER_FILENAME " -level \"");

    // Change command for starting level test
    WriteMemPtr(0x00447973 + 1, CmdBuf);
    WriteMemPtr(0x00447CB9 + 1, CmdBuf);

    // Zero first argument for CreateProcess call
    AsmWritter(0x00447B32, 0x00447B39).nop();
    AsmWritter(0x00447B32).xor_(asm_regs::eax, asm_regs::eax);
    AsmWritter(0x00448024, 0x0044802B).nop();
    AsmWritter(0x00448024).xor_(asm_regs::eax, asm_regs::eax);

    // InitInstance hook
    AsmWritter(0x00482C84).call(CEditorApp__InitInstance_AfterHook);

    return 1; // success
}

BOOL WINAPI DllMain(HINSTANCE Instance, [[maybe_unused]] DWORD Reason, [[maybe_unused]] LPVOID Reserved)
{
    g_Module = (HMODULE)Instance;
    DisableThreadLibraryCalls(Instance);
    return TRUE;
}
