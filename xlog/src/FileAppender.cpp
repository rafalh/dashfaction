
#include <xlog/FileAppender.h>

xlog::FileAppender::FileAppender(const std::string& filename, bool append, bool flush) :
    flush_(flush)
{
    std::ios_base::openmode m = std::ios_base::out;
    if (append)
        m |= std::ios_base::app;
    else
        m |= std::ios_base::trunc;
    file_.open(filename, m);
}

void xlog::FileAppender::append([[maybe_unused]] xlog::Level level, const std::string& formatted_message)
{
#ifndef XLOG_SINGLE_THREADED
    std::lock_guard<std::mutex> lock(mutex_);
#endif
    file_ << formatted_message << '\n';
    if (flush_)
        file_.flush();
}

void xlog::FileAppender::flush()
{
#ifndef XLOG_SINGLE_THREADED
    std::lock_guard<std::mutex> lock(mutex_);
#endif
    file_.flush();
}
