#pragma once

#include <log/LogLevel.h>
#include <string>

namespace logging
{

class Appender
{
public:
    virtual void append(LogLevel lvl, const std::string& loggerName, const std::string& str) = 0;
    virtual ~Appender() {}
};

} // namespace logging
