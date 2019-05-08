#pragma once

#include <log/Appender.h>
#include <mutex>

namespace logging
{

class BaseAppender : public Appender
{
public:
    void append(Level lvl, const std::string& logger_name, const std::string& str) override;
    virtual void append(Level lvl, const std::string& str) = 0;

private:
    std::mutex mutex;
};

} // namespace logging
