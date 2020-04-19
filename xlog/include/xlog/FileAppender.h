#pragma once

#include <fstream>
#include <mutex>
#include <xlog/Appender.h>

namespace xlog
{

class FileAppender : public Appender
{
public:
    FileAppender(const std::string& filename, bool append = true, bool flush = true);

protected:
    void append(Level level, const std::string& formatted_message) override;
    void flush() override;

private:
    std::ofstream file_;
    bool flush_;
#ifndef XLOG_SINGLE_THREADED
    std::mutex mutex_;
#endif
};

}
