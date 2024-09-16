#pragma once

#include <exception>
#include <string>
#include <format>

#define THROW_EXCEPTION(...) throw Exception({__FILE__, __LINE__}, __VA_ARGS__)

class Exception : public std::exception
{
public:
    struct SourceLocation
    {
        const char* file;
        int line;
    };

    template<typename... Args>
    Exception(SourceLocation loc, std::format_string<Args...> fmt, Args&&... args)
    {
        m_what = std::format(fmt, std::forward<Args>(args)...);
        std::format_to(std::back_inserter(m_what), " in {}:{}", loc.file, loc.line);
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_what.c_str();
    }

private:
    std::string m_what;
};
