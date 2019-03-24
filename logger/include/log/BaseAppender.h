#pragma once

#include <log/Appender.h>
#include <mutex>

namespace logging
{
    class BaseAppender : public Appender
    {
    public:
        void append(LogLevel lvl, const std::string &loggerName, const std::string &str) override;
        virtual void append(LogLevel lvl, const std::string &str) = 0;

    private:
        std::mutex mutex;
    };
}
