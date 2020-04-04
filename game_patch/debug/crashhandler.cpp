#include "crashhandler.h"
#include <log/Logger.h>
#include <windows.h>
#include <signal.h>
#include <cstring>

static WCHAR g_module_path[MAX_PATH];
static LPTOP_LEVEL_EXCEPTION_FILTER g_old_exception_filter;

void CrashHandlerProcessException(PEXCEPTION_POINTERS exception_ptrs, DWORD thread_id)
{
    ERR("Unhandled exception: ExceptionAddress=0x%p ExceptionCode=0x%lX",
        exception_ptrs->ExceptionRecord->ExceptionAddress, exception_ptrs->ExceptionRecord->ExceptionCode);
    for (unsigned i = 0; i < exception_ptrs->ExceptionRecord->NumberParameters; ++i)
        ERR("ExceptionInformation[%d]=0x%lX", i, exception_ptrs->ExceptionRecord->ExceptionInformation[i]);

    static HANDLE process_handle = nullptr;
    static HANDLE event_handle = nullptr;
    do {
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &process_handle, 0, TRUE,
                             DUPLICATE_SAME_ACCESS))
            break;

        static SECURITY_ATTRIBUTES sec_attribs = {sizeof(sec_attribs), nullptr, TRUE};
        event_handle = CreateEventW(&sec_attribs, FALSE, FALSE, nullptr);
        if (!event_handle)
            break;

        static STARTUPINFOW startup_info;
        std::memset(&startup_info, 0, sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);

        static WCHAR cmd_line[256];
        std::swprintf(cmd_line, ARRAYSIZE(cmd_line), L"%ls\\CrashHandler.exe 0x%p 0x%p %lu 0x%p", g_module_path,
                      exception_ptrs, process_handle, thread_id, event_handle);

        static PROCESS_INFORMATION proc_info;
        if (!CreateProcessW(nullptr, cmd_line, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup_info, &proc_info)) {
            ERR("Failed to start CrashHandler process - CreateProcessW failed with error %ls", cmd_line);
            break;
        }

        WaitForSingleObject(event_handle, 5000);
        CloseHandle(proc_info.hProcess);
        CloseHandle(proc_info.hThread);
    } while (false);

    if (process_handle)
        CloseHandle(process_handle);
    if (event_handle)
        CloseHandle(event_handle);
}

static LONG WINAPI CrashHandlerExceptionFilter(PEXCEPTION_POINTERS exception_ptrs)
{
    CrashHandlerProcessException(exception_ptrs, GetCurrentThreadId());
    return g_old_exception_filter ? g_old_exception_filter(exception_ptrs) : EXCEPTION_EXECUTE_HANDLER;
}

static void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
                                    unsigned line, [[maybe_unused]] uintptr_t reserved)
{
    ERR("Invalid parameter detected in function %ls. File: %ls Line: %d", function, file, line);
    ERR("Expression: %ls", expression);
    RaiseException(custom_exceptions::invalid_parameter, EXCEPTION_NONCONTINUABLE_EXCEPTION, 0, nullptr);
    ExitProcess(0);
}

static void SignalHandler(int signal_number)
{
    ERR("Abort signal (%d) received!", signal_number);
    RaiseException(custom_exceptions::abort, EXCEPTION_NONCONTINUABLE_EXCEPTION, 0, nullptr);
    ExitProcess(0);
}

void CrashHandlerInit(HMODULE module_handle)
{
    GetModuleFileNameW(module_handle, g_module_path, ARRAYSIZE(g_module_path));

    WCHAR* filename = wcsrchr(g_module_path, L'\\');
    if (filename)
        *filename = L'\0';

    g_old_exception_filter = SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);

    _set_invalid_parameter_handler(InvalidParameterHandler);

    signal(SIGABRT, &SignalHandler);
}

void CrashHandlerCleanup()
{
    SetUnhandledExceptionFilter(g_old_exception_filter);
}
