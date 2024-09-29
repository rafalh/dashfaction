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

    protected:
        std::string prepare(Level level, const std::string& logger_name) const override;
    };
}
