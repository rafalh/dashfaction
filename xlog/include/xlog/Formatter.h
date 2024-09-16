#pragma once

#include <string>
#include <format>
#include <xlog/Level.h>

#ifdef XLOG_PRINTF
#include <cstdarg>
#endif

namespace xlog
{

class Logger;

class Formatter
{
public:
    virtual ~Formatter() = default;

    template<typename... Args>
    std::string format(Level level, const std::string& logger_name, std::format_string<Args...> fmt, Args&&... args) const
    {
        std::string buf = prepare(level, logger_name);
        std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
        return buf;
    }

#ifdef XLOG_PRINTF
    std::string vformat(Level level, const std::string& logger_name, const char* fmt, std::va_list args) const
    {
        std::string buf = prepare(level, logger_name);
        char message[256];
        std::vsnprintf(message, sizeof(message), fmt, args);
        buf += message;
        return buf;
    }
#endif

protected:
    virtual std::string prepare(Level level, const std::string& logger_name) const = 0;
};

}
