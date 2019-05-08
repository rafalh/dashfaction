
#include <log/FileAppender.h>

using namespace logging;

void FileAppender::append([[maybe_unused]] Level lvl, const std::string& str)
{
    if (!m_file.is_open()) {
        std::ios_base::openmode m = std::ios_base::out;
        if (m_append)
            m |= std::ios_base::app;
        else
            m |= std::ios_base::trunc;
        m_file.open(m_filename, m);
    }

    m_file << str << std::endl;
    if (m_flush)
        m_file.flush();
}
