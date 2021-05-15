#pragma once

#include <vector>
#include <memory>
#include <string>
#include <xlog/Level.h>
#include <xlog/Appender.h>

namespace xlog
{

class LoggerConfig
{
public:
    LoggerConfig();

    [[nodiscard]] static LoggerConfig& get()
    {
        static LoggerConfig conf;
        return conf;
    }

    LoggerConfig& add_appender(std::unique_ptr<Appender>&& appender)
    {
        appenders_.push_back(std::move(appender));
        return *this;
    }

    template<typename T, typename... A>
    LoggerConfig& add_appender(A... args)
    {
        appenders_.push_back(std::make_unique<T>(args...));
        return *this;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Appender>>& get_appenders() const
    {
        return appenders_;
    }

    void set_root_name(const std::string& root_name)
    {
        root_name_ = root_name;
    }

    [[nodiscard]] const std::string& get_root_name() const
    {
        return root_name_;
    }

    void set_default_level(Level level)
    {
        default_level_ = level;
    }

    [[nodiscard]] Level get_default_level() const
    {
        return default_level_;
    }

    void flush_appenders()
    {
        for (auto& appender : appenders_) {
            appender->flush();
        }
    }

private:
    std::string root_name_;
    Level default_level_;
    std::vector<std::unique_ptr<Appender>> appenders_;
};

}
