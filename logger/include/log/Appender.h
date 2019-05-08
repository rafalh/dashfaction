#pragma once

#include <log/LogLevel.h>
#include <string>

namespace logging
{

class Appender
{
public:
    virtual void append(Level lvl, const std::string& logger_name, const std::string& str) = 0;
    virtual ~Appender() {}
};

} // namespace logging
