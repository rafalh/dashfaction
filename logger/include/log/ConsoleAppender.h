#pragma once

#include <iostream>
#include <log/BaseAppender.h>

namespace logging
{

class ConsoleAppender : public BaseAppender
{
public:
    ConsoleAppender(Level max_stderr_level = Level::warning) :
        m_max_stderr_level(max_stderr_level) {}

    void append(Level lvl, const std::string& str) override
    {
        if (lvl <= m_max_stderr_level)
            std::cerr << str << std::endl;
        else
            std::cout << str << std::endl;
    }

private:
    Level m_max_stderr_level;
};

} // namespace logging
