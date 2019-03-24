#include <log/BaseAppender.h>
#include <ctime>
#include <sstream>

static const char *LEVEL_PREFIX[] = {
    "FATAL: ", "ERROR: ", "WARNING: ", "INFO: ", "TRACE: "
};

using namespace logging;

void BaseAppender::append(LogLevel lvl, const std::string &loggerName, const std::string &str)
{
    const char *lvlStr = LEVEL_PREFIX[static_cast<int>(lvl)];

    std::ostringstream buf;
    std::lock_guard<std::mutex> lock(mutex);

    time_t now;
    time(&now);
    struct tm nowInfo;
#ifdef _WIN32
    localtime_s(&nowInfo, &now);
#else
    localtime_r(&now, &nowInfo);
#endif
    char timeBuf[32] = "";
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &nowInfo);
    buf << "[" << timeBuf << "] " << lvlStr;
    if (!loggerName.empty())
        buf << loggerName << ' ';
    buf << str;

    append(lvl, buf.str());
}
