#pragma once

#include <cstdarg>
#include <cstdio>
#include <exception>
#include <string>

#define THROW_EXCEPTION(fmt, ...) throw Exception(fmt " in %s:%u", ##__VA_ARGS__, __FILE__, __LINE__)

class Exception : public std::exception
{
public:
    Exception(const char* pszFormat, ...)
    {
        va_list ap;
        va_start(ap, pszFormat);
        char szBuf[256];
        std::vsnprintf(szBuf, sizeof(szBuf), pszFormat, ap);
        va_end(ap);
        m_strWhat = szBuf;
    }

    ~Exception() throw() {}

    const char* what() const throw()
    {
        return m_strWhat.c_str();
    }

private:
    std::string m_strWhat;
};
