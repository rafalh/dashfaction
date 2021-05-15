#pragma once

#include <string>
#include <cstdarg>
#include <xlog/Level.h>

namespace xlog
{

class Logger;

class Formatter
{
public:
    virtual std::string format(Level level, const std::string& logger_name, std::string_view message) = 0;
    virtual std::string vformat(Level level, const std::string& logger_name, const char* format, std::va_list args) = 0;
    virtual ~Formatter() = default;
};

}
