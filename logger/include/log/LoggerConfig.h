#pragma once

#include <fstream>
#include <mutex>
#include <vector>
#include <stdarg.h>
#include <log/LogLevel.h>
#include <log/Appender.h>

namespace logging
{
    class LoggerConfig
    {
    public:
        LoggerConfig();

        static LoggerConfig &root()
        {
            static LoggerConfig conf;
            return conf;
        }

        void addAppender(std::unique_ptr<Appender> &&appender)
        {
            appenders.push_back(std::move(appender));
        }

        void output(LogLevel lvl, const std::string &loggerName, const std::string &str);
        void outputFormatted(LogLevel lvl, const std::string &loggerName, const char *fmt, va_list args);

        void setRootName(const std::string &name)
        {
            rootName = name;
        }

        const std::string &getRootName() const
        {
            return rootName;
        }

    private:
        std::string outputFileName;
        std::ofstream outputFile;
        std::mutex mutex;
        std::string rootName;
        std::vector<std::unique_ptr<Appender>> appenders;
        bool flush, append;
    };
}
