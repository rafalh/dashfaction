#pragma once

// Define helper macro
#ifdef __GNUC__
#define XLOG_ATTRIBUTE_FORMAT_PRINTF(format_arg, rest_arg) __attribute__ ((format (printf, format_arg, rest_arg)))
#else
#define XLOG_ATTRIBUTE_FORMAT_PRINTF(format_arg, rest_arg)
#endif

#ifndef XLOG_NO_DISCARD_TRACE
#define XLOG_DISCARD_TRACE
#endif

#include <cstdarg>
#include <cstdio>
#include <xlog/Level.h>
#include <xlog/Logger.h>

namespace xlog
{
    inline void log(Level level, const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    inline void log(Level level, const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlog(level, format, args);
        va_end(args);
    }

    inline LogStream log(Level level)
    {
        return Logger::root().log(level);
    }

    inline void error(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void error(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlog(Level::error, format, args);
        va_end(args);
    }

    inline auto error()
    {
        return Logger::root().error();
    }

    inline void warn(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void warn(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlog(Level::warn, format, args);
        va_end(args);
    }

    inline auto warn()
    {
        return Logger::root().warn();
    }

    inline void info(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void info(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlog(Level::info, format, args);
        va_end(args);
    }

    inline auto info()
    {
        return Logger::root().info();
    }

    inline void debug(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void debug(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlog(Level::debug, format, args);
        va_end(args);
    }

    inline auto debug()
    {
        return Logger::root().debug();
    }

    inline void trace(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
#ifndef XLOG_DISCARD_TRACE
    inline void trace(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlog(Level::trace, format, args);
        va_end(args);
    }
#else
    inline void trace([[ maybe_unused ]] const char *format, ...)
    {
    }
#endif

    inline auto trace()
    {
        return Logger::root().trace();
    }
}

// Undefine helper macro
#undef XLOG_ATTRIBUTE_FORMAT_PRINTF

// Backward compatibility
#ifndef XLOG_NO_MACROS

#define ERR(...) xlog::error(__VA_ARGS__)
#define WARN(...) xlog::warn(__VA_ARGS__)
#define INFO(...) xlog::info(__VA_ARGS__)
#define TRACE(...) xlog::trace(__VA_ARGS__)

#define LOG_ONCE(lvl, ...) do { \
    static bool skip = false; \
    if (!skip) \
        lvl(__VA_ARGS__); \
        skip = true; \
    } while (false)
#define ERR_ONCE(...) LOG_ONCE(xlog::error, __VA_ARGS__)
#define WARN_ONCE(...) LOG_ONCE(xlog::warn, __VA_ARGS__)
#define INFO_ONCE(...) LOG_ONCE(xlog::info, __VA_ARGS__)
#define TRACE_ONCE(...) LOG_ONCE(xlog::trace, __VA_ARGS__)

#endif // XLOG_NO_MACROS
