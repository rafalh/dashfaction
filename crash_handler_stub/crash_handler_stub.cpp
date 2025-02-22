#define XLOG_PRINTF

#include "crash_handler_stub.h"
#include "crash_handler_stub/custom_exceptions.h"
#include <xlog/xlog.h>
#include <windows.h>
#include <csignal>
#include <cwchar>

static WCHAR g_module_path[MAX_PATH];
static LPTOP_LEVEL_EXCEPTION_FILTER g_old_exception_filter;
static CrashHandlerConfig g_config;

void CrashHandlerStubProcessException(PEXCEPTION_POINTERS exception_ptrs, DWORD thread_id)
{
    xlog::error("Unhandled exception: ExceptionAddress={} ExceptionCode=0x{:x}",
        exception_ptrs->ExceptionRecord->ExceptionAddress, exception_ptrs->ExceptionRecord->ExceptionCode);
    for (unsigned i = 0; i < exception_ptrs->ExceptionRecord->NumberParameters; ++i)
        xlog::error("ExceptionInformation[{}]=0x{:x}", i, exception_ptrs->ExceptionRecord->ExceptionInformation[i]);
    xlog::flush();

    HANDLE process_handle = nullptr;
    HANDLE event_handle = nullptr;
    do {
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &process_handle, 0, TRUE,
                             DUPLICATE_SAME_ACCESS))
            break;

        SECURITY_ATTRIBUTES sec_attribs = {sizeof(sec_attribs), nullptr, TRUE};
        event_handle = CreateEventW(&sec_attribs, FALSE, FALSE, nullptr);
        if (!event_handle)
            break;

        STARTUPINFOW startup_info;
        ZeroMemory(&startup_info, sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);

        WCHAR cmd_line[256];
        std::swprintf(cmd_line, std::size(cmd_line), L"%ls\\CrashHandler.exe 0x%p 0x%p %lu 0x%p 0x%p", g_module_path,
                      exception_ptrs, process_handle, thread_id, event_handle, &g_config);
        xlog::infof("Running crash handler: %ls", cmd_line);

        PROCESS_INFORMATION proc_info;
        if (!CreateProcessW(nullptr, cmd_line, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup_info, &proc_info)) {
            xlog::errorf("Failed to start CrashHandler process - CreateProcessW %ls failed with error %lu", cmd_line, GetLastError());
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
    xlog::flush();
}

static LONG WINAPI CrashHandlerExceptionFilter(PEXCEPTION_POINTERS exception_ptrs)
{
    CrashHandlerStubProcessException(exception_ptrs, GetCurrentThreadId());
    return g_old_exception_filter ? g_old_exception_filter(exception_ptrs) : EXCEPTION_EXECUTE_HANDLER;
}

static void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
                                    unsigned line, [[maybe_unused]] uintptr_t reserved)
{
    xlog::errorf("Invalid parameter detected in function %ls. File: %ls Line: %d", function, file, line);
    xlog::errorf("Expression: %ls", expression);
    xlog::flush();
    RaiseException(custom_exceptions::invalid_parameter, EXCEPTION_NONCONTINUABLE_EXCEPTION, 0, nullptr);
    ExitProcess(0);
}

static void SignalHandler(int signal_number)
{
    xlog::error("Abort signal ({}) received!", signal_number);
    xlog::flush();
    RaiseException(custom_exceptions::abort, EXCEPTION_NONCONTINUABLE_EXCEPTION, 0, nullptr);
    ExitProcess(0);
}

void CrashHandlerStubInstall(const CrashHandlerConfig& config)
{
    g_config = config;
    GetModuleFileNameW(config.this_module_handle, g_module_path, std::size(g_module_path));

    WCHAR* filename = wcsrchr(g_module_path, L'\\');
    if (filename)
        *filename = L'\0';

    g_old_exception_filter = SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);

    _set_invalid_parameter_handler(InvalidParameterHandler);

    signal(SIGABRT, &SignalHandler);
}

void CrashHandlerStubUninstall()
{
    SetUnhandledExceptionFilter(g_old_exception_filter);
}
