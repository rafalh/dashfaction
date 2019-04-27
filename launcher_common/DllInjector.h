#pragma once

#include "Process.h"

class DllInjector
{
private:
    Process& m_process;

public:
    DllInjector(Process& process) :
        m_process(process)
    {}

    HMODULE inject_dll(const char* dll_filename, const char* init_fun_name, int timeout);

private:
    DWORD run_remote_fun(FARPROC fun_ptr, void* arg, int timeout);
    DWORD load_library_remote(const char* dll_path, int timeout);
};
