
#include <log/FileAppender.h>

using namespace logging;

void FileAppender::append([[maybe_unused]] LogLevel lvl, const std::string& str)
{
    if (!m_fileStream.is_open()) {
        std::ios_base::openmode m = std::ios_base::out;
        if (m_append)
            m |= std::ios_base::app;
        else
            m |= std::ios_base::trunc;
        m_fileStream.open(m_filename, m);
    }

    m_fileStream << str << std::endl;
    if (m_flush)
        m_fileStream.flush();
}
