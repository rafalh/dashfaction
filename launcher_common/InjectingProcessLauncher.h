#pragma once

#include <windows.h>
#include "Process_.h"
#include "Thread.h"
#include "DllInjector.h"

class ProcessTerminatedError : public std::runtime_error
{
public:
    ProcessTerminatedError() : std::runtime_error("process has terminated") {}
};

class InjectingProcessLauncher
{
private:
    Process m_process;
    Thread m_thread;
    bool m_resumed = false;

public:
    InjectingProcessLauncher(const char* executable, const char* work_dir, const char* command_line,
                             STARTUPINFO& startup_info, int timeout);

    ~InjectingProcessLauncher()
    {
        try {
            if (!m_resumed && m_process)
                m_process.terminate(1);
        }
        catch (const std::exception& e) {
            std::fprintf(stderr, "%s\n", e.what());
        }
    }

    void inject_dll(const char* dll_filename, const char* init_fun_name, int timeout)
    {
        DllInjector injector{m_process};
        injector.inject_dll(dll_filename, init_fun_name, timeout);
    }

    void resume_main_thread()
    {
        m_thread.resume();
        m_resumed = true;
    }

    void wait(int timeout)
    {
        m_process.wait(timeout);
    }

private:
    void wait_for_process_initialization(uintptr_t entry_point, int timeout);
};
