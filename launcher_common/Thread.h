#pragma once

#include <windows.h>
#include "Win32Handle.h"
#include "Win32Error.h"

class Thread
{
private:
    Win32Handle m_handle;

public:
    Thread() {}
    Thread(Win32Handle&& handle) : m_handle(std::move(handle)) {}

    void suspend()
    {
        if (SuspendThread(m_handle) == static_cast<DWORD>(-1))
            THROW_WIN32_ERROR();
    }

    void resume()
    {
        if (ResumeThread(m_handle) == static_cast<DWORD>(-1))
            THROW_WIN32_ERROR();
    }

    void wait(DWORD timeout) const
    {
        DWORD wait_result = WaitForSingleObject(m_handle, timeout);
        if (wait_result != WAIT_OBJECT_0) {
            if (wait_result == WAIT_TIMEOUT)
                THROW_EXCEPTION("timeout");
            else
                THROW_WIN32_ERROR();
        }
    }

    void get_context(CONTEXT* ctx)
    {
        if (!GetThreadContext(m_handle, ctx))
            THROW_WIN32_ERROR();
    }

    DWORD get_exit_code() const
    {
        DWORD exit_code;
        if (!GetExitCodeThread(m_handle, &exit_code))
            THROW_WIN32_ERROR();
        return exit_code;
    }

    operator bool() const
    {
        return m_handle != nullptr;
    }

    const Win32Handle& get_handle() const
    {
        return m_handle;
    }
};
