#include "CLoggerConfig.h"
#include <iostream>
#include <sstream>

CLoggerConfig::CLoggerConfig():
    flush(false) {}

void CLoggerConfig::setOutputFile(const std::string &filename, bool append)
{
    std::ios_base::openmode m = std::ios_base::out;
    if (append)
        m |= std::ios_base::app;
    else
        m |= std::ios_base::trunc;
    outputFile.open(filename, m);
}

void CLoggerConfig::setFlushEnabled(bool enabled)
{
    flush = enabled;
}

void CLoggerConfig::output(ELogLevel lvl, const std::string &loggerName, const std::string &str)
{
    const char *levels[] = {
        "FATAL: ", "ERROR: ", "WARNING: ", "INFO: ", "TRACE: "
    };
    const char *lvlStr = levels[static_cast<int>(lvl)];

    std::ostringstream buf;
    std::lock_guard<std::mutex> lock(mutex);

    time_t now;
    time(&now);
    struct tm nowInfo;
    localtime_s(&nowInfo, &now);
    char timeBuf[32] = "";
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &nowInfo);
    buf << "[" << timeBuf;
    if (!component.empty() || !loggerName.empty())
    {
        buf << " " << component;
        if (!component.empty() && !loggerName.empty())
            buf << "/";
        buf << loggerName << "";
    }
    buf << "] " << lvlStr << str << std::endl;

    if (outputFile.is_open())
    {
        outputFile << buf.str();
        if (flush)
            outputFile.flush();
    }

    std::cerr << buf.str();
    if (flush)
        std::cerr.flush();
}
