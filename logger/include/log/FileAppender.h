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

    void append(Level lvl, const std::string& str) override;

private:
    std::string m_filename;
    std::ofstream m_file;
    bool m_append, m_flush;
};

} // namespace logging
