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
        Logger(const std::string& name, Level level = Level::trace) :
            name_(name), level_(level)
        {}

        static Logger& root()
        {
            static Logger root_logger(LoggerConfig::get().get_root_name(), LoggerConfig::get().get_default_level());
            return root_logger;
        }

        void log(Level level, const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(3, 4)
        {
            std::va_list args;
            va_start(args, format);
            vlog(level, format, args);
            va_end(args);
        }

        void vlog(Level level, const char* format, va_list args)
        {
            if (level <= level_) {
                for (auto& appender : LoggerConfig::get().get_appenders()) {
                    appender->vappend(level, name_, format, args);
                }
            }
        }

#ifdef XLOG_STREAMS
        LogStream log(Level level)
        {
            return LogStream(level, name_, level_);
        }
#endif

        void error(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::error, format, args);
            va_end(args);
        }

#ifdef XLOG_STREAMS
        LogStream error()
        {
            return log(Level::error);
        }
#endif

        void warn(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::warn, format, args);
            va_end(args);
        }

#ifdef XLOG_STREAMS
        LogStream warn()
        {
            return log(Level::warn);
        }
#endif

        void info(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::info, format, args);
            va_end(args);
        }

#ifdef XLOG_STREAMS
        LogStream info()
        {
            return log(Level::info);
        }
#endif

        void debug(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::debug, format, args);
            va_end(args);
        }

#ifdef XLOG_STREAMS
        LogStream debug()
        {
            return log(Level::debug);
        }
#endif

        void trace([[ maybe_unused ]] const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
#ifndef XLOG_DISCARD_TRACE
            std::va_list args;
            va_start(args, format);
            vlog(Level::trace, format, args);
            va_end(args);
#endif
        }

#ifdef XLOG_STREAMS
        auto trace()
        {
#ifndef XLOG_DISCARD_TRACE
            return log(Level::trace);
#else
            return NullStream();
#endif
        }
#endif

        const std::string& name() const
        {
            return name_;
        }

        void set_level(Level level)
        {
            level_ = level;
        }
    };
}
