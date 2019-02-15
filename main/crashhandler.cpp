#include "stdafx.h"
#include "crashhandler.h"

static WCHAR g_ModulePath[MAX_PATH];
static LPTOP_LEVEL_EXCEPTION_FILTER g_OldExceptionFilter;

static LONG WINAPI CrashHandlerExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
    static HANDLE hProcess = NULL, hEvent = NULL;
    static STARTUPINFOW StartupInfo;
    static PROCESS_INFORMATION ProcInfo;
    static WCHAR CmdLine[256];

    do
    {
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &hProcess, 0, TRUE, DUPLICATE_SAME_ACCESS))
            break;

        static SECURITY_ATTRIBUTES SecAttribs = { sizeof(SecAttribs), NULL, TRUE };
        hEvent = CreateEventW(&SecAttribs, FALSE, FALSE, NULL);
        if (!hEvent)
            break;

        memset(&StartupInfo, 0, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        swprintf(CmdLine, ARRAYSIZE(CmdLine), L"%ls\\CrashHandler.exe 0x%p 0x%p %lu 0x%p", g_ModulePath, pExceptionInfo, hProcess, GetCurrentThreadId(), hEvent);
        if (!CreateProcessW(NULL, CmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcInfo))
            break;

        WaitForSingleObject(hEvent, 2000);
        CloseHandle(ProcInfo.hProcess);
        CloseHandle(ProcInfo.hThread);
    } while (false);
    
    if (hProcess)
        CloseHandle(hProcess);
    if (hEvent)
        CloseHandle(hEvent);

    return g_OldExceptionFilter ? g_OldExceptionFilter(pExceptionInfo) : EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandlerInit(HMODULE hModule)
{
    GetModuleFileNameW(hModule, g_ModulePath, ARRAYSIZE(g_ModulePath));
    WCHAR *Filename = wcsrchr(g_ModulePath, L'\\');
    if (Filename)
        *Filename = L'\0';

    g_OldExceptionFilter = SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
}

void CrashHandlerCleanup(void)
{
    SetUnhandledExceptionFilter(g_OldExceptionFilter);
}
