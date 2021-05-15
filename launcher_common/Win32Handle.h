#pragma once

#include <windows.h>
#include <utility>

class Win32Handle
{
private:
    HANDLE m_handle = nullptr;

public:
    Win32Handle() = default;

    Win32Handle(HANDLE handle) :
        m_handle(handle)
    {}

    ~Win32Handle()
    {
        if (m_handle)
            CloseHandle(m_handle);
    }

    Win32Handle(const Win32Handle&) = delete; // copy constructor
    Win32Handle& operator=(const Win32Handle&) = delete; // assignment operator

    Win32Handle(Win32Handle&& other) noexcept : // move constructor
        m_handle(std::exchange(other.m_handle, nullptr))
    {}

    Win32Handle& operator=(Win32Handle&& other) noexcept // move assignment
    {
        std::swap(m_handle, other.m_handle);
        return *this;
    }

    Win32Handle &operator=(HANDLE handle)
    {
        if (m_handle)
            CloseHandle(m_handle);
        m_handle = handle;
        return *this;
    }

    operator HANDLE() const
    {
        return m_handle;
    }
};
