#pragma once

#include <sstream>
#include "LogLevel.h"
#include "LoggerConfig.h"

namespace logging
{
    class LogStream
    {
    public:
        LogStream(LogLevel lvl, const std::string &loggerName, LoggerConfig &loggerConfig) :
            level(lvl), loggerName(loggerName), loggerConfig(loggerConfig) {}

        LogStream(const LogStream &e) :
            level(e.level), loggerName(loggerName), loggerConfig(e.loggerConfig) {}

        template<typename T> LogStream &operator<<(const T &v)
        {
            stream << v;
            return *this;
        }

        ~LogStream()
        {
            loggerConfig.output(level, loggerName, stream.str());
        }

    private:
        LogLevel level;
        const std::string &loggerName;
        LoggerConfig &loggerConfig;
        std::ostringstream stream;
    };
}