#pragma once

#include <windows.h>
#include <cstring>

struct CrashHandlerConfig
{
    HMODULE this_module_handle = nullptr;
    char output_dir[MAX_PATH] = {0};
    char log_file[MAX_PATH] = {0};
    char app_name[256] = {0};
    char known_modules[8][32] = {0};
    int num_known_modules = 0;

    void add_known_module(const char* module_name)
    {
        std::strcpy(known_modules[num_known_modules++], module_name);
    }
};

void CrashHandlerStubInstall(const CrashHandlerConfig& config);
void CrashHandlerStubUninstall();
void CrashHandlerStubProcessException(PEXCEPTION_POINTERS exception_ptrs, DWORD thread_id);
