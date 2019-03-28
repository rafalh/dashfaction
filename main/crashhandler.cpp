#include "stdafx.h"
#include "crashhandler.h"
#include <windows.h>
#include <log/Logger.h>

static WCHAR g_ModulePath[MAX_PATH];
static LPTOP_LEVEL_EXCEPTION_FILTER g_OldExceptionFilter;

static LONG WINAPI CrashHandlerExceptionFilter(PEXCEPTION_POINTERS exception_ptrs)
{
    static HANDLE process_handle = NULL, event_handle = NULL;
    static STARTUPINFOW startup_info;
    static PROCESS_INFORMATION proc_info;
    static WCHAR cmd_line[256];

    ERR("Unhandled exception: ExceptionAddress=0x%X ExceptionCode=0x%X",
        exception_ptrs->ExceptionRecord->ExceptionAddress, exception_ptrs->ExceptionRecord->ExceptionCode);
    for (unsigned i = 0; i < exception_ptrs->ExceptionRecord->NumberParameters; ++i)
        ERR("ExceptionInformation[%d]=0x%X", i, exception_ptrs->ExceptionRecord->ExceptionInformation[i]);

    do
    {
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &process_handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
            break;

        static SECURITY_ATTRIBUTES sec_attribs = { sizeof(sec_attribs), NULL, TRUE };
        event_handle = CreateEventW(&sec_attribs, FALSE, FALSE, NULL);
        if (!event_handle)
            break;

        memset(&startup_info, 0, sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);
        swprintf(cmd_line, ARRAYSIZE(cmd_line), L"%ls\\CrashHandler.exe 0x%p 0x%p %lu 0x%p", g_ModulePath, exception_ptrs, process_handle, GetCurrentThreadId(), event_handle);
        if (!CreateProcessW(NULL, cmd_line, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &proc_info))
        {
            ERR("Failed to start CrashHandler process - CreateProcessW failed with error %ls", cmd_line);
            break;
        }

        WaitForSingleObject(event_handle, 2000);
        CloseHandle(proc_info.hProcess);
        CloseHandle(proc_info.hThread);
    } while (false);

    if (process_handle)
        CloseHandle(process_handle);
    if (event_handle)
        CloseHandle(event_handle);

    return g_OldExceptionFilter ? g_OldExceptionFilter(exception_ptrs) : EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandlerInit(HMODULE module_handle)
{
    GetModuleFileNameW(module_handle, g_ModulePath, ARRAYSIZE(g_ModulePath));

    WCHAR *filename = wcsrchr(g_ModulePath, L'\\');
    if (filename)
        *filename = L'\0';

    g_OldExceptionFilter = SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
}

void CrashHandlerCleanup(void)
{
    SetUnhandledExceptionFilter(g_OldExceptionFilter);
}
