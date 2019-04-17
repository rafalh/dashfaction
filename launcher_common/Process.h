#pragma once

#include <windows.h>

class Process
{
public:
    Process(HANDLE process_handle) : m_handle(process_handle) {}

    Process(int pid);
    ~Process();

    template<typename T = void*, typename T2>
    T ExecFun(const char* module_name, const char* fun_name, T2 arg = 0, unsigned timeout = 3000)
    {
        return reinterpret_cast<T>(ExecFunInternal(module_name, fun_name, reinterpret_cast<void*>(arg), timeout));
    }

    void* SaveData(const void* data, size_t len);

    void* SaveString(const char* value)
    {
        return SaveData(value, strlen(value) + 1);
    }

    void FreeMem(void* remote_ptr);
    HMODULE FindModule(const char* module_name);
    void ReadMem(void* dest_ptr, const void* remote_src_ptr, size_t len);

private:
    HANDLE m_handle;

    void* ExecFunInternal(const char* module_name, const char* fun_name, void* arg, unsigned timeout);
    void* RemoteGetProcAddress(HMODULE module, const char* fun_name);
};
