#include <log/LoggerConfig.h>
#include <iostream>
#include <sstream>
#ifdef _WIN32
# include <Windows.h>
#endif

using namespace logging;

LoggerConfig::LoggerConfig() :
    outputFileName("app.log"), flush(false), append(false) {}

void LoggerConfig::outputFormatted(LogLevel lvl, const std::string &loggerName, const char *fmt, va_list args)
{
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    output(lvl, loggerName, buf);
}

void LoggerConfig::output(LogLevel lvl, const std::string &loggerName, const std::string &str)
{
    for (std::unique_ptr<Appender> &appender : appenders)
        appender->append(lvl, loggerName, str);
}
