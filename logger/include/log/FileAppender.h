#pragma once

#include <fstream>
#include <log/BaseAppender.h>

namespace logging
{

class FileAppender : public BaseAppender
{
public:
    FileAppender(const std::string& filename, bool append = true, bool flush = true) :
        m_filename(filename), m_append(append), m_flush(flush)
    {}

    virtual void append(LogLevel lvl, const std::string& str) override;

private:
    std::string m_filename;
    std::ofstream m_fileStream;
    bool m_append, m_flush;
};

} // namespace logging
