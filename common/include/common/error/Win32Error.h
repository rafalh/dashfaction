#pragma once

#include <stdexcept>
#include <common/error/Exception.h>
#include <common/error/error-utils.h>

class Win32Error : public std::exception
{
public:
    Win32Error(const char* msg = "") :
        Win32Error(GetLastError(), msg)
    {}

    Win32Error(DWORD error, const char* msg = "") :
        m_error(error),
        m_what(msg)
    {
        if (!m_what.empty())
            m_what += ": ";
        m_what += "win32 error " + std::to_string(m_error);
        auto error_desc = get_win32_error_description(m_error);
        m_what += "\n" + error_desc;
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_what.c_str();
    }

    [[nodiscard]] DWORD error() const
    {
        return m_error;
    }

private:
    DWORD m_error;
    std::string m_what;
};


#define S(x) #x
#define S_(x) S(x)
#define S_LINE S_(__LINE__)

// Macro for throwing error together with location
#define THROW_WIN32_ERROR(...) throw Win32Error(__FILE__ ":" S_LINE " " __VA_ARGS__)
