#pragma once

#include <fstream>
#include <log/Appender.h>
#include <log/LogLevel.h>
#include <memory>
#include <mutex>
#include <stdarg.h>
#include <vector>

namespace logging
{

class LoggerConfig
{
public:
    LoggerConfig();

    static LoggerConfig& root()
    {
        static LoggerConfig conf;
        return conf;
    }

    void add_appender(std::unique_ptr<Appender>&& appender)
    {
        appenders.push_back(std::move(appender));
    }

    void output(Level lvl, const std::string& logger_name, const std::string& str);
    void output_formatted(Level lvl, const std::string& logger_name, const char* fmt, va_list args);

    void set_root_name(const std::string& name)
    {
        root_name = name;
    }

    const std::string& get_root_name() const
    {
        return root_name;
    }

private:
    std::string output_file_name;
    std::ofstream output_file;
    std::mutex mutex;
    std::string root_name;
    std::vector<std::unique_ptr<Appender>> appenders;
    bool flush, append;
};

} // namespace logging
