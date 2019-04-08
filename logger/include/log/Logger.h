#pragma once

#include <log/LogStream.h>
#include <log/LoggerConfig.h>
#include <log/NullStream.h>
#include <string>

#ifndef LOGGER_NO_FORMAT
#include <stdarg.h>
#endif

#ifndef LOGGER_NO_ROOT_MACROS
#define ERR(...) logging::Logger::root().err(__VA_ARGS__)
#define WARN(...) logging::Logger::root().warn(__VA_ARGS__)
#define INFO(...) logging::Logger::root().info(__VA_ARGS__)
#define TRACE(...) logging::Logger::root().trace(__VA_ARGS__)
#endif // LOGGER_NO_ROOT_MACROS

#define LOGGER_DISCARD_TRACE

// clang-format off
#define LOGGER_DEFINE_LEVEL_METHODS(methodName, levelConstant) \
    LogStream methodName() { return get(levelConstant); } \
    void methodName(const char *fmt, ...) \
    { \
        va_list args; \
        va_start(args, fmt); \
        vprintf(levelConstant, fmt, args); \
        va_end(args); \
    }
#define LOGGER_DEFINE_LEVEL_METHODS_EMPTY(methodName, levelConstant) \
    NullStream methodName() { return NullStream(); } \
    void methodName([[maybe_unused]] const char *fmt, ...) {}
// clang-format on

namespace logging
{
class Logger
{
public:
    Logger(const std::string& name = std::string()) :
        name(name), conf(LoggerConfig::root())
    {}

    Logger(const std::string& name, LoggerConfig& conf) :
        name(name), conf(conf)
    {}

    static Logger& root()
    {
        static Logger rootLogger(LoggerConfig::root().getRootName());
        return rootLogger;
    }

    LogStream get(LogLevel lvl)
    {
        return LogStream(lvl, name, conf);
    }

    void vprintf(LogLevel lvl, const char* fmt, va_list args)
    {
        conf.outputFormatted(lvl, name, fmt, args);
    }

    LOGGER_DEFINE_LEVEL_METHODS(err, LOG_LVL_ERROR);
    LOGGER_DEFINE_LEVEL_METHODS(warn, LOG_LVL_WARNING);
    LOGGER_DEFINE_LEVEL_METHODS(info, LOG_LVL_INFO);

#ifndef LOGGER_DISCARD_TRACE
    LOGGER_DEFINE_LEVEL_METHODS(trace, LOG_LVL_TRACE);
#else
    LOGGER_DEFINE_LEVEL_METHODS_EMPTY(trace, LOG_LVL_TRACE);
#endif

private:
    std::string name;
    LoggerConfig& conf;

    // no assignment operator
    Logger& operator=(const Logger&) = delete;
};
} // namespace logging

#undef LOGGER_DEFINE_LEVEL_METHODS
#undef LOGGER_DEFINE_LEVEL_METHODS_EMPTY
