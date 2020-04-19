#pragma once

#include <xlog/Level.h>
#include <xlog/LoggerConfig.h>
#include <xlog/LogStream.h>
#include <xlog/NullStream.h>

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

        LogStream log(Level level)
        {
            return LogStream(level, name_, level_);
        }

        void error(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::error, format, args);
            va_end(args);
        }

        LogStream error()
        {
            return log(Level::error);
        }

        void warn(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::warn, format, args);
            va_end(args);
        }

        LogStream warn()
        {
            return log(Level::warn);
        }

        void info(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::info, format, args);
            va_end(args);
        }

        LogStream info()
        {
            return log(Level::info);
        }

        void debug(const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
            std::va_list args;
            va_start(args, format);
            vlog(Level::debug, format, args);
            va_end(args);
        }

        LogStream debug()
        {
            return log(Level::debug);
        }

        void trace([[ maybe_unused ]] const char *format, ...) XLOG_ATTRIBUTE_FORMAT_PRINTF(2, 3)
        {
#ifndef XLOG_DISCARD_TRACE
            std::va_list args;
            va_start(args, format);
            vlog(Level::trace, format, args);
            va_end(args);
#endif
        }

        auto trace()
        {
#ifndef XLOG_DISCARD_TRACE
            return log(Level::trace);
#else
            return NullStream();
#endif
        }

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
