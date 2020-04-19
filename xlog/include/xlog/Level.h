#pragma once

#include <cstring>

namespace xlog
{
    enum class Level
    {
        error = 0,
        warn = 1,
        info = 2,
        debug = 3,
        trace = 4,
    };

    inline Level parse_level(const char* str)
    {
        if (!std::strcmp(str, "ERROR")) {
            return Level::error;
        }
        if (!std::strcmp(str, "WARN")) {
            return Level::warn;
        }
        if (!std::strcmp(str, "INFO")) {
            return Level::info;
        }
        if (!std::strcmp(str, "DEBUG")) {
            return Level::debug;
        }
        // Default level is trace
        return Level::trace;
    }
}
