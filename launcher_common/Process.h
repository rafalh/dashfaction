#pragma once

#include <windows.h>
#include <common/error/Win32Error.h>
#include "Win32Handle.h"
#include "Thread.h"

class RemoteMemoryPtr
{
private:
    void* m_ptr;
    HANDLE m_process;

public:
    RemoteMemoryPtr(void* ptr, HANDLE process) :
        m_ptr(ptr), m_process(process)
    {}

    ~RemoteMemoryPtr()
    {
        if (m_ptr)
            VirtualFreeEx(m_process, m_ptr, 0, MEM_RELEASE);
    }

    RemoteMemoryPtr(const RemoteMemoryPtr&) = delete; // copy constructor
    RemoteMemoryPtr& operator=(const RemoteMemoryPtr&) = delete; // assignment operator

    RemoteMemoryPtr(RemoteMemoryPtr&& other) : // move constructor
        m_ptr(std::exchange(other.m_ptr, nullptr)),
        m_process(std::exchange(other.m_process, nullptr))
    {}

    RemoteMemoryPtr& operator=(RemoteMemoryPtr&& other) // move assignment operator
    {
        std::swap(m_ptr, other.m_ptr);
        std::swap(m_process, other.m_process);
        return *this;
    }

    operator void*() const
    {
        return m_ptr;
    }
};

class Process
{
private:
    Win32Handle m_handle;

public:
    Process() = default;
    Process(Win32Handle&& handle) : m_handle(std::move(handle)) {}

    [[nodiscard]] RemoteMemoryPtr alloc_mem(size_t size)
    {
        void* remote_ptr = VirtualAllocEx(m_handle, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!remote_ptr)
            THROW_WIN32_ERROR();
        return {remote_ptr, m_handle};
    }

    void read_mem(void* dest_ptr, const void* src_ptr_remote, size_t len)
    {
        if (!ReadProcessMemory(m_handle, src_ptr_remote, dest_ptr, len, nullptr))
            THROW_WIN32_ERROR();
    }

    void write_mem(void* dest_ptr_remote, const void* src_ptr, size_t len)
    {
        if (!WriteProcessMemory(m_handle, dest_ptr_remote, src_ptr, len, nullptr))
            THROW_WIN32_ERROR();
    }

    void wait(DWORD timeout) const
    {
        DWORD wait_result = WaitForSingleObject(m_handle, timeout);
        if (wait_result != WAIT_OBJECT_0) {
            if (wait_result == WAIT_TIMEOUT) {
                THROW_EXCEPTION("timeout");
            }
            THROW_WIN32_ERROR();
        }
    }

    Thread create_remote_thread(LPTHREAD_START_ROUTINE fun_ptr, void* arg)
    {
        Win32Handle thread_handle =
        ::CreateRemoteThread(m_handle, nullptr, 0, fun_ptr, arg, 0, nullptr);
        if (!thread_handle)
            THROW_WIN32_ERROR();

        return {std::move(thread_handle)};
    }

    void terminate(DWORD exit_code)
    {
        if (!TerminateProcess(m_handle, exit_code))
            THROW_WIN32_ERROR();
    }

    operator bool() const
    {
        return m_handle != nullptr;
    }

    [[nodiscard]] const Win32Handle& get_handle() const
    {
        return m_handle;
    }
};

class ProcessMemoryProtection
{
private:
    HANDLE m_process_handle;
    void* m_ptr;
    size_t m_size;
    DWORD m_old_protect;

public:
    ProcessMemoryProtection(Process& process, void* ptr, size_t size, unsigned protect) :
        m_process_handle(process.get_handle()), m_ptr(ptr), m_size(size)
    {
        if (!VirtualProtectEx(m_process_handle, m_ptr, m_size, protect, &m_old_protect))
            THROW_WIN32_ERROR();
    }

    ~ProcessMemoryProtection()
    {
        if (!VirtualProtectEx(m_process_handle, m_ptr, m_size, m_old_protect, &m_old_protect))
            std::fprintf(stderr, "VirtualProtectEx failed to unprotect memory: %ld\n", GetLastError());
    }

    ProcessMemoryProtection(const ProcessMemoryProtection&) = delete; // copy constructor
    ProcessMemoryProtection& operator=(const ProcessMemoryProtection&) = delete; // assignment operator
};
