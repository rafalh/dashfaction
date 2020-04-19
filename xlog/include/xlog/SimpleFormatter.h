#pragma once

#include <xlog/Formatter.h>

namespace xlog
{
    class SimpleFormatter : public Formatter
    {
        bool include_time_;
        bool include_level_;
        bool include_logger_name_;

    public:
        SimpleFormatter(bool include_time = true, bool include_level = true, bool include_logger_name = true) :
            include_time_(include_time), include_level_(include_level), include_logger_name_(include_logger_name)
        {}

        virtual std::string format(Level level, const std::string& logger_name, std::string_view message) override;
        virtual std::string vformat(Level level, const std::string& logger_name, const char* format, std::va_list args) override;
    };
}
