#include "stdafx.h"
#include "crashhandler.h"
#include <windows.h>
#include <log/Logger.h>

static WCHAR g_ModulePath[MAX_PATH];
static LPTOP_LEVEL_EXCEPTION_FILTER g_OldExceptionFilter;

static LONG WINAPI CrashHandlerExceptionFilter(PEXCEPTION_POINTERS ExceptionInfo)
{
    static HANDLE Process_handle = NULL, Event_handle = NULL;
    static STARTUPINFOW StartupInfo;
    static PROCESS_INFORMATION ProcInfo;
    static WCHAR CmdLine[256];

    ERR("Unhandled exception: ExceptionAddress=0x%X ExceptionCode=0x%X",
        ExceptionInfo->ExceptionRecord->ExceptionAddress, ExceptionInfo->ExceptionRecord->ExceptionCode);
    for (unsigned i = 0; i < ExceptionInfo->ExceptionRecord->NumberParameters; ++i)
        ERR("ExceptionInformation[%d]=0x%X", i, ExceptionInfo->ExceptionRecord->ExceptionInformation[i]);

    do
    {
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &Process_handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
            break;

        static SECURITY_ATTRIBUTES SecAttribs = { sizeof(SecAttribs), NULL, TRUE };
        Event_handle = CreateEventW(&SecAttribs, FALSE, FALSE, NULL);
        if (!Event_handle)
            break;

        memset(&StartupInfo, 0, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        swprintf(CmdLine, ARRAYSIZE(CmdLine), L"%ls\\CrashHandler.exe 0x%p 0x%p %lu 0x%p", g_ModulePath, ExceptionInfo, Process_handle, GetCurrentThreadId(), Event_handle);
        if (!CreateProcessW(NULL, CmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcInfo))
        {
            ERR("Failed to start CrashHandler process - CreateProcessW failed with error %ls", CmdLine);
            break;
        }

        WaitForSingleObject(Event_handle, 2000);
        CloseHandle(ProcInfo.hProcess);
        CloseHandle(ProcInfo.hThread);
    } while (false);

    if (Process_handle)
        CloseHandle(Process_handle);
    if (Event_handle)
        CloseHandle(Event_handle);

    return g_OldExceptionFilter ? g_OldExceptionFilter(ExceptionInfo) : EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandlerInit(HMODULE Module_handle)
{
    GetModuleFileNameW(Module_handle, g_ModulePath, ARRAYSIZE(g_ModulePath));

    WCHAR *Filename = wcsrchr(g_ModulePath, L'\\');
    if (Filename)
        *Filename = L'\0';

    g_OldExceptionFilter = SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
}

void CrashHandlerCleanup(void)
{
    SetUnhandledExceptionFilter(g_OldExceptionFilter);
}
