#pragma once

#include <cstdarg>
#include <cstdio>
#include <exception>
#include <string>
#include <memory>

#define THROW_EXCEPTION(...) throw Exception({__FILE__, __LINE__}, __VA_ARGS__)

class Exception : public std::exception
{
public:
    struct SourceLocation
    {
        const char* file;
        int line;
    };

    Exception(SourceLocation loc, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        int size = 1;
        size += std::vsnprintf(nullptr, 0, format, args);
        size += std::snprintf(nullptr, 0, " in %s:%u", loc.file, loc.line);
        va_end(args);

        std::unique_ptr<char[]> buf(new char[size]);

        va_start(args, format);
        int pos = 0;
        pos += std::vsnprintf(buf.get() + pos, size - pos, format, args);
        std::snprintf(buf.get() + pos, size - pos, " in %s:%u", loc.file, loc.line);
        va_end(args);

        m_what = buf.get();
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_what.c_str();
    }

private:
    std::string m_what;
};
