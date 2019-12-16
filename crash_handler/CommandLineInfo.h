#pragma once

#include <wxx_wincore.h>

class CommandLineInfo
{
public:
    bool Parse()
    {
        std::vector<CString> args = Win32xx::GetCommandLineArgs();
        if (args.size() < 5) {
            return false;
        }
        m_exception_ptrs = reinterpret_cast<EXCEPTION_POINTERS*>(std::strtoull(args[1], nullptr, 0));
        m_process_handle = reinterpret_cast<HANDLE>(std::strtoull(args[2], nullptr, 0));
        m_thread_id = static_cast<DWORD>(std::strtoull(args[3], nullptr, 0));
        m_event = reinterpret_cast<HANDLE>(std::strtoull(args[4], nullptr, 0));
        return true;
    }

    EXCEPTION_POINTERS* GetExceptionPtrs() const
    {
        return m_exception_ptrs;
    }

    HANDLE GetProcessHandle() const
    {
        return m_process_handle;
    }

    DWORD GetThreadId() const
    {
        return m_thread_id;
    }

    HANDLE GetEvent() const
    {
        return m_event;
    }

private:
    EXCEPTION_POINTERS* m_exception_ptrs = nullptr;
    HANDLE m_process_handle = nullptr;
    DWORD m_thread_id = 0;
    HANDLE m_event = nullptr;
};
