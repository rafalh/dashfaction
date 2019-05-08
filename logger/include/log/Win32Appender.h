#pragma once

#include <log/BaseAppender.h>

namespace logging
{

class Win32Appender : public BaseAppender
{
    void append(Level lvl, const std::string& str) override;
};

} // namespace logging
