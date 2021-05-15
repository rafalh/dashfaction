#pragma once

#include <xlog/Level.h>
#include <xlog/LoggerConfig.h>
#include <sstream>

namespace xlog
{

class LogStream : public std::ostringstream
{
public:
    LogStream(Level level, const std::string& logger_name, Level logger_level) :
        level_(level), logger_name_(logger_name), logger_level_(logger_level)
    {}

    LogStream(LogStream&& e) :
        level_(e.level_), logger_name_(std::move(e.logger_name_)), logger_level_(e.logger_level_)
    {}

    LogStream(const LogStream&) = delete; // copy constructor
    LogStream& operator=(const LogStream&) = delete; // assignment operator
    LogStream& operator=(LogStream&& other) = delete; // move assignment

    ~LogStream() override
    {
        if (level_ <= logger_level_) {
            for (const auto& appender : LoggerConfig::get().get_appenders()) {
                appender->append(level_, logger_name_, str());
            }
        }
    }

private:
    Level level_;
    const std::string& logger_name_;
    Level logger_level_;
};

}
