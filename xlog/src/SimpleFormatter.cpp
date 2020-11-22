#include <xlog/SimpleFormatter.h>
#include <string>
#include <string_view>
#include <windows.h>

std::string xlog::SimpleFormatter::format(xlog::Level level, const std::string& logger_name, std::string_view message)
{
    static auto start_ticks = GetTickCount();
    std::string buf;
    buf.reserve(message.size() + 32);

    if (include_time_) {
        char time_buf[16] = "";
        auto now = (GetTickCount() - start_ticks) / 1000;
        unsigned h = (now / 3600) % 24;
        unsigned m = (now / 60) % 60;
        unsigned s = now % 60;
        std::sprintf(time_buf, "[%02u:%02u:%02u] ", h, m, s);
        buf += time_buf;
    }

    if (include_level_) {
        static const char* level_prefix[] = {"ERROR: ", "WARN: ", "INFO: ", "DEBUG: ", "TRACE: "};
        const char* lvl_str = level_prefix[static_cast<int>(level)];
        buf += lvl_str;
    }

    if (include_logger_name_ && !logger_name.empty()) {
        buf += logger_name;
        buf += ' ';
    }

    buf += message;

    return buf;
}

std::string xlog::SimpleFormatter::vformat(Level level, const std::string& logger_name, const char* format, std::va_list args)
{
    char buf[256];
    std::vsnprintf(buf, sizeof(buf), format, args);
    return this->format(level, logger_name, std::string_view{buf});
}
