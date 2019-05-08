#define WIN32_LEAN_AND_MEAN

#include <log/Win32Appender.h>
#include <windows.h>

using namespace logging;

void Win32Appender::append([[maybe_unused]] Level lvl, const std::string& str)
{
    OutputDebugStringA(str.c_str());
}
