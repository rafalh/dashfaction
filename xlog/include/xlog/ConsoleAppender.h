#pragma once

#include <iostream>
#include <mutex>
#include <xlog/Appender.h>

namespace xlog
{

class ConsoleAppender : public Appender
{
public:
    ConsoleAppender(Level max_stderr_level = Level::warn) :
        max_stderr_level_(max_stderr_level) {}

protected:
    void append(Level level, const std::string& formatted_message) override
    {
#ifndef XLOG_SINGLE_THREADED
        std::lock_guard<std::mutex> lock(mutex_);
#endif
        if (level <= max_stderr_level_)
            std::cerr << formatted_message << '\n';
        else
            std::cout << formatted_message << '\n';
    }

    void flush() override
    {
        std::cerr.flush();
        std::cout.flush();
    }

private:
    Level max_stderr_level_;
#ifndef XLOG_SINGLE_THREADED
    std::mutex mutex_;
#endif
};

}
