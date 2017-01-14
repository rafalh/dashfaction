#pragma once

#include <log/BaseAppender.h>

namespace logging
{
    class Win32Appender : public BaseAppender
    {
        virtual void append(LogLevel lvl, const std::string &str);
    };
}
