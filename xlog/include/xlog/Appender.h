#pragma once

#include <xlog/Level.h>
#include <xlog/SimpleFormatter.h>
#include <memory>
#include <string>
#include <string_view>

namespace xlog
{

class Logger;

class Appender
{
public:
    Appender() : formatter_(new SimpleFormatter())
    {}

    template<typename... Args>
    void append(Level level, const std::string& logger_name, std::format_string<Args...> fmt, Args&&... args)
    {
        if (level <= level_) {
            auto formatted = formatter_->format(level, logger_name, fmt, std::forward<Args>(args)...);
            append(level, formatted);
        }
    }

#ifdef XLOG_PRINTF
    void vappendf(Level level, const std::string& logger_name, const char* fmt, std::va_list args)
    {
        if (level <= level_) {
            auto formatted = formatter_->vformat(level, logger_name, fmt, args);
            append(level, formatted);
        }
    }
#endif

    template<typename T, typename... A>
    void set_formatter(A... args)
    {
        formatter_ = std::make_unique<T>(args...);
    }

    void set_level(Level level)
    {
        level_ = level;
    }

    virtual void flush() {}
    virtual ~Appender() = default;

protected:
    virtual void append(Level level, const std::string& formatted_message) = 0;

private:
    std::unique_ptr<Formatter> formatter_;
    Level level_ = Level::trace;
};

}
