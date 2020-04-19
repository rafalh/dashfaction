#pragma once

#include <xlog/Appender.h>

namespace xlog
{

class Win32Appender : public Appender
{
    void append(Level level, const std::string& formatted_message) override;
};

}
