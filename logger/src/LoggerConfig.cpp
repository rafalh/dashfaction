#include <iostream>
#include <log/LoggerConfig.h>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace logging;

LoggerConfig::LoggerConfig() :
    output_file_name("app.log"), flush(false), append(false)
{}

void LoggerConfig::output_formatted(Level lvl, const std::string& logger_name, const char* fmt, va_list args)
{
    char buf[256];
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    output(lvl, logger_name, buf);
}

void LoggerConfig::output(Level lvl, const std::string& logger_name, const std::string& str)
{
    for (std::unique_ptr<Appender>& appender : appenders) {
        appender->append(lvl, logger_name, str);
    }
}
