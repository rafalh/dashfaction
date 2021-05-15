#include <xlog/LoggerConfig.h>
#include <xlog/Level.h>
#include <string_view>

inline xlog::Level parse_level(const char* str)
{
    std::string_view str_sv{str};
    if (str_sv == "ERROR") {
        return xlog::Level::error;
    }
    if (str_sv == "WARN") {
        return xlog::Level::warn;
    }
    if (str_sv == "INFO") {
        return xlog::Level::info;
    }
    if (str_sv == "DEBUG") {
        return xlog::Level::debug;
    }
    // Default level is trace
    return xlog::Level::trace;
}

xlog::LoggerConfig::LoggerConfig()
{
    const char* level_name = std::getenv("XLOG_LEVEL");
    if (level_name) {
        default_level_ = parse_level(level_name);
    }
    else {
#ifdef NDEBUG
        default_level_ = Level::info;
#else
        default_level_ = Level::debug;
#endif
    }
}
