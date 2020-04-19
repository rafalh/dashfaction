#include <xlog/SimpleFormatter.h>
#include <sstream>
#include <ctime>

std::string xlog::SimpleFormatter::format(xlog::Level level, const std::string& logger_name, std::string_view message)
{
    std::ostringstream ss;

    if (include_time_) {
        std::time_t now;
        std::time(&now);
        std::tm now_info;
    #ifdef _WIN32
        localtime_s(&now_info, &now);
    #else
        localtime_r(&now, &now_info);
    #endif
        char time_buf[32] = "";
        std::strftime(time_buf, std::size(time_buf), "%H:%M:%S", &now_info);
        ss << "[" << time_buf << "] ";
    }

    if (include_level_) {
        static const char* level_prefix[] = {"ERROR: ", "WARN: ", "INFO: ", "DEBUG: ", "TRACE: "};
        const char* lvl_str = level_prefix[static_cast<int>(level)];
        ss << lvl_str;
    }

    if (include_logger_name_ && !logger_name.empty())
        ss << logger_name << ' ';
    ss << message;

    return ss.str();
}

std::string xlog::SimpleFormatter::vformat(Level level, const std::string& logger_name, const char* format, std::va_list args)
{
    char buf[256];
    std::vsnprintf(buf, sizeof(buf), format, args);
    return this->format(level, logger_name, std::string_view{buf});
}
