#pragma once

#include <log/BaseAppender.h>
#include <iostream>

namespace logging
{
    class ConsoleAppender : public BaseAppender
    {
    public:
        ConsoleAppender(LogLevel maxStdErrLevel = LOG_LVL_WARNING) :
            m_maxStdErrLevel(maxStdErrLevel) {}

        virtual void append(LogLevel lvl, const std::string &str)
        {
            if (lvl <= m_maxStdErrLevel)
                std::cerr << str << std::endl;
            else
                std::cout << str << std::endl;
        }

    private:
        LogLevel m_maxStdErrLevel;
    };
}
