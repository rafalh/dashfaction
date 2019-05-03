#pragma once

#include <cstdarg>
#include <cstdio>
#include <exception>
#include <string>
#include <memory>

#define THROW_EXCEPTION(fmt, ...) throw Exception(fmt " in %s:%u", ##__VA_ARGS__, __FILE__, __LINE__)

class Exception : public std::exception
{
public:
    Exception(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        int size = std::vsnprintf(nullptr, 0, format, args) + 1;
        va_end(args);

        std::unique_ptr<char[]> buf(new char[size]);

        va_start(args, format);
        std::vsnprintf(buf.get(), size, format, args);
        va_end(args);

        m_what = buf.get();
    }

    ~Exception() throw() {}

    const char* what() const throw()
    {
        return m_what.c_str();
    }

private:
    std::string m_what;
};
