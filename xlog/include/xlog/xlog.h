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

#include <xlog/Level.h>
#include <xlog/Logger.h>

#ifdef XLOG_PRINTF
#include <cstdarg>
#endif

namespace xlog
{
    template<typename... Args>
    inline void log(Level level, std::format_string<Args...> fmt, Args&&... args)
    {
        Logger::root().log(level, fmt, std::forward<Args>(args)...);
    }

#ifdef XLOG_PRINTF
    inline void log(Level level, const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    inline void log(Level level, const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlogf(level, format, args);
        va_end(args);
    }
#endif

#ifdef XLOG_STREAMS
    inline LogStream log(Level level)
    {
        return Logger::root().log(level);
    }
#endif

    template<typename... Args>
    inline void error(std::format_string<Args...> fmt, Args&&... args)
    {
        Logger::root().log(Level::error, fmt, std::forward<Args>(args)...);
    }

#ifdef XLOG_PRINTF
    inline void errorf(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void errorf(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlogf(Level::error, format, args);
        va_end(args);
    }
#endif

#ifdef XLOG_STREAMS
    inline auto error()
    {
        return Logger::root().error();
    }
#endif

    template<typename... Args>
    inline void warn(std::format_string<Args...> fmt, Args&&... args)
    {
        Logger::root().log(Level::warn, fmt, std::forward<Args>(args)...);
    }

#ifdef XLOG_PRINTF
    inline void warnf(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void warnf(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlogf(Level::warn, format, args);
        va_end(args);
    }
#endif

#ifdef XLOG_STREAMS
    inline auto warn()
    {
        return Logger::root().warn();
    }
#endif

    template<typename... Args>
    inline void info(std::format_string<Args...> fmt, Args&&... args)
    {
        Logger::root().log(Level::info, fmt, std::forward<Args>(args)...);
    }

#ifdef XLOG_PRINTF
    inline void infof(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void infof(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlogf(Level::info, format, args);
        va_end(args);
    }
#endif

#ifdef XLOG_STREAMS
    inline auto info()
    {
        return Logger::root().info();
    }
#endif

    template<typename... Args>
    inline void debug(std::format_string<Args...> fmt, Args&&... args)
    {
        Logger::root().log(Level::debug, fmt, std::forward<Args>(args)...);
    }

#ifdef XLOG_PRINTF
    inline void debugf(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
    inline void debugf(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlogf(Level::debug, format, args);
        va_end(args);
    }
#endif

#ifdef XLOG_STREAMS
    inline auto debug()
    {
        return Logger::root().debug();
    }
#endif

    template<typename... Args>
    inline void tracef(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(1, 2);
#ifndef XLOG_DISCARD_TRACE
    template<typename... Args>
    inline void trace(std::format_string<Args...> fmt, Args&&... args)
    {
        Logger::root().log(Level::trace, fmt, std::forward<Args>(args)...);
    }

#ifdef XLOG_PRINTF
    inline void tracef(const char *format, ...)
    {
        std::va_list args;
        va_start(args, format);
        Logger::root().vlogf(Level::trace, format, args);
        va_end(args);
    }
#endif

#else
    inline void trace([[ maybe_unused ]] const char *format, ...)
    {
    }

#ifdef XLOG_PRINTF
    inline void tracef([[ maybe_unused ]] const char *format, ...)
    {
    }
#endif

#endif

#ifdef XLOG_STREAMS
    inline auto trace()
    {
        return Logger::root().trace();
    }
#endif

    inline void flush()
    {
        for (const auto& appender : LoggerConfig::get().get_appenders()) {
            appender->flush();
        }
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

#define LOG_ONCE(lvl, ...) \
    do { \
        static bool skip = false; \
        if (!skip) {\
            lvl(__VA_ARGS__); \
            skip = true; \
        } \
    } while (false)
#define ERR_ONCE(...) LOG_ONCE(xlog::error, __VA_ARGS__)
#define WARN_ONCE(...) LOG_ONCE(xlog::warn, __VA_ARGS__)
#define INFO_ONCE(...) LOG_ONCE(xlog::info, __VA_ARGS__)
#define TRACE_ONCE(...) LOG_ONCE(xlog::trace, __VA_ARGS__)
#define DEBUG_ONCE(...) LOG_ONCE(xlog::debug, __VA_ARGS__)

#endif // XLOG_NO_MACROS
