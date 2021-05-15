#pragma once

#include <xlog/Level.h>
#include <xlog/SimpleFormatter.h>
#include <memory>
#include <string>
#include <string_view>
#include <cstdarg>

namespace xlog
{

class Logger;

class Appender
{
public:
    Appender() : formatter_(new SimpleFormatter())
    {}

    void append(Level level, const std::string& logger_name, std::string_view message)
    {
        if (level <= level_) {
            auto formatted = formatter_->format(level, logger_name, message);
            append(level, formatted);
        }
    }

    void vappend(Level level, const std::string& logger_name, const char* format, std::va_list args)
    {
        if (static_cast<int>(level) <= static_cast<int>(level_)) {
            auto formatted = formatter_->vformat(level, logger_name, format, args);
            append(level, formatted);
        }
    }

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
