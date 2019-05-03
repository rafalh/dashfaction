#pragma once

#include <stdexcept>
#include <common/Exception.h>
#include <common/ErrorUtils.h>

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

    const char* what() const throw()
    {
        return m_what.c_str();
    }

    DWORD error() const
    {
        return m_error;
    }

private:
    DWORD m_error;
    std::string m_what;
};


#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)

// Macro for throwing error together with location
#define THROW_WIN32_ERROR(...) throw Win32Error(__FILE__ ":" S__LINE__ " " __VA_ARGS__)
