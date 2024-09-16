#pragma once

#include <xlog/Level.h>
#include <xlog/LoggerConfig.h>
#ifdef XLOG_STREAMS
#include <xlog/LogStream.h>
#include <xlog/NullStream.h>
#endif

namespace xlog
{
    class Logger
    {
        std::string name_;
        Level level_;

    public:
        Logger(std::string name, Level level = Level::trace) :
            name_(std::move(name)), level_(level)
        {}

        static Logger& root()
        {
            static Logger root_logger(LoggerConfig::get().get_root_name(), LoggerConfig::get().get_default_level());
            return root_logger;
        }

        template<typename... Args>
        void log(Level level, std::format_string<Args...> fmt, Args&&... args)
        {
            if (level <= level_) {
                for (const auto& appender : LoggerConfig::get().get_appenders()) {
                    appender->append(level, name_, fmt, std::forward<Args>(args)...);
                }
            }
        }

#ifdef XLOG_PRINTF
        void logf(Level level, const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(3, 4)
        {
            std::va_list args;
            va_start(args, format);
            vlogf(level, format, args);
            va_end(args);
        }

        void vlogf(Level level, const char* format, va_list args)
        {
            if (level <= level_) {
                for (const auto& appender : LoggerConfig::get().get_appenders()) {
                    appender->vappendf(level, name_, format, args);
                }
            }
        }
#endif

#ifdef XLOG_STREAMS
        LogStream log(Level level)
        {
            return LogStream(level, name_, level_);
        }
#endif

        template<typename... Args>
        void error(std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::error, fmt, std::forward<Args>(args)...);
        }


#ifdef XLOG_STREAMS
        LogStream error()
        {
            return log(Level::error);
        }
#endif

        template<typename... Args>
        void warn(std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::warn, fmt, std::forward<Args>(args)...);
        }

#ifdef XLOG_STREAMS
        LogStream warn()
        {
            return log(Level::warn);
        }
#endif

        template<typename... Args>
        void info(std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::info, fmt, std::forward<Args>(args)...);
        }

#ifdef XLOG_STREAMS
        LogStream info()
        {
            return log(Level::info);
        }
#endif

        template<typename... Args>
        void debug(std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::debug, fmt, std::forward<Args>(args)...);
        }

#ifdef XLOG_STREAMS
        LogStream debug()
        {
            return log(Level::debug);
        }
#endif

        template<typename... Args>
        void trace(std::format_string<Args...> fmt, Args&&... args)
        {
#ifndef XLOG_DISCARD_TRACE
            log(Level::trace, fmt, std::forward<Args>(args)...);
#endif
        }

#ifdef XLOG_STREAMS
        auto trace() // NOLINT(readability-convert-member-functions-to-static)
        {
#ifndef XLOG_DISCARD_TRACE
            return log(Level::trace);
#else
            return NullStream();
#endif
        }
#endif

        [[nodiscard]] const std::string& name() const
        {
            return name_;
        }

        void set_level(Level level)
        {
            level_ = level;
        }
    };
}
