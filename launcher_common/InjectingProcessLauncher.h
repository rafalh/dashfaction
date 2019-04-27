#pragma once

#include <windows.h>
#include "Process.h"
#include "Thread.h"
#include "DllInjector.h"

class InjectingProcessLauncher
{
private:
    Process m_process;
    Thread m_thread;

public:
    InjectingProcessLauncher(const char* executable, const char* work_dir, const char* command_line,
                             STARTUPINFO& startup_info, int timeout);

    void inject_dll(const char* dll_filename, const char* init_fun_name, int timeout)
    {
        DllInjector injector{m_process};
        injector.inject_dll(dll_filename, init_fun_name, timeout);
    }

    void resume_main_thread()
    {
        m_thread.resume();
    }

    void wait(int timeout)
    {
        m_process.wait(timeout);
    }

private:
    void wait_for_process_initialization(uintptr_t entry_point, int timeout);
};
