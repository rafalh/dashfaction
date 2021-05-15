#pragma once

#include <windows.h>
#include <cstring>

struct CrashHandlerConfig
{
    HMODULE this_module_handle = nullptr;
    char output_dir[MAX_PATH] = {};
    char log_file[MAX_PATH] = {};
    char app_name[256] = {};
    char known_modules[8][32] = {};
    int num_known_modules = 0;

    void add_known_module(const char* module_name)
    {
        std::strcpy(known_modules[num_known_modules++], module_name);
    }
};

void CrashHandlerStubInstall(const CrashHandlerConfig& config);
void CrashHandlerStubUninstall();
void CrashHandlerStubProcessException(PEXCEPTION_POINTERS exception_ptrs, DWORD thread_id);
