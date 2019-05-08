#include <ctime>
#include <log/BaseAppender.h>
#include <sstream>



using namespace logging;

void BaseAppender::append(Level lvl, const std::string& logger_name, const std::string& str)
{
    static const char* level_prefix[] = {"FATAL: ", "ERROR: ", "WARNING: ", "INFO: ", "TRACE: "};
    const char* lvl_str = level_prefix[static_cast<int>(lvl)];

    std::ostringstream buf;
    std::lock_guard<std::mutex> lock(mutex);

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
    buf << "[" << time_buf << "] " << lvl_str;
    if (!logger_name.empty())
        buf << logger_name << ' ';
    buf << str;

    append(lvl, buf.str());
}
