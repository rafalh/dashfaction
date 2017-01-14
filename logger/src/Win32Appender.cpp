#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <log/Win32Appender.h>

using namespace logging;

void Win32Appender::append(LogLevel lvl, const std::string &str)
{
    OutputDebugStringA(str.c_str());
}
