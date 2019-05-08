#pragma once

#include <log/LogStream.h>
#include <log/LoggerConfig.h>
#include <log/NullStream.h>
#include <string>

#ifndef LOGGER_NO_FORMAT
#include <cstdarg>
#endif

#ifndef LOGGER_NO_ROOT_MACROS
#define ERR(...) logging::Logger::root().err(__VA_ARGS__)
#define WARN(...) logging::Logger::root().warn(__VA_ARGS__)
#define INFO(...) logging::Logger::root().info(__VA_ARGS__)
#define TRACE(...) logging::Logger::root().trace(__VA_ARGS__)
#endif // LOGGER_NO_ROOT_MACROS

#define LOGGER_DISCARD_TRACE

#ifdef __GNUC__
#define LOGGER_FORMAT_ATTRIBUTE __attribute__ ((format (printf, 2, 3)))
#else
#define LOGGER_FORMAT_ATTRIBUTE
#endif

// clang-format off
#define LOGGER_DEFINE_LEVEL_METHODS(method_name, level_constant) \
    LogStream method_name() { return get(level_constant); } \
    LOGGER_FORMAT_ATTRIBUTE void method_name(const char *fmt, ...) \
    { \
        va_list args; \
        va_start(args, fmt); \
        vprintf(level_constant, fmt, args); \
        va_end(args); \
    }
#define LOGGER_DEFINE_LEVEL_METHODS_EMPTY(method_name, level_constant) \
    NullStream method_name() { return NullStream(); } \
    LOGGER_FORMAT_ATTRIBUTE void method_name([[maybe_unused]] const char *fmt, ...) {}
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
        static Logger root_logger(LoggerConfig::root().get_root_name());
        return root_logger;
    }

    LogStream get(Level lvl)
    {
        return LogStream(lvl, name, conf);
    }

    void vprintf(Level lvl, const char* fmt, va_list args)
    {
        conf.output_formatted(lvl, name, fmt, args);
    }

    LOGGER_DEFINE_LEVEL_METHODS(err, Level::error);
    LOGGER_DEFINE_LEVEL_METHODS(warn, Level::warning);
    LOGGER_DEFINE_LEVEL_METHODS(info, Level::info);

#ifndef LOGGER_DISCARD_TRACE
    LOGGER_DEFINE_LEVEL_METHODS(trace, Level::trace);
#else
    LOGGER_DEFINE_LEVEL_METHODS_EMPTY(trace, Level::trace);
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
