#pragma once

#include "LogLevel.h"
#include "LoggerConfig.h"
#include <sstream>

namespace logging
{

class LogStream
{
public:
    LogStream(Level lvl, const std::string& logger_name, LoggerConfig& logger_config) :
        level(lvl), logger_name(logger_name), logger_config(logger_config)
    {}

    LogStream(LogStream&& e) :
        level(e.level), logger_name(std::move(e.logger_name)), logger_config(e.logger_config)
    {}

    LogStream(const LogStream&) = delete; // copy constructor
    LogStream& operator=(const LogStream&) = delete; // assignment operator
    LogStream& operator=(LogStream&& other) = delete; // move assignment

    ~LogStream()
    {
        logger_config.output(level, logger_name, stream.str());
    }

    template<typename T>
    LogStream& operator<<(const T& v)
    {
        stream << v;
        return *this;
    }

private:
    Level level;
    const std::string& logger_name;
    LoggerConfig& logger_config;
    std::ostringstream stream;
};

} // namespace logging
