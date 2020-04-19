#define WIN32_LEAN_AND_MEAN

#include <xlog/Win32Appender.h>
#include <windows.h>

void xlog::Win32Appender::append([[maybe_unused]] xlog::Level lvl, const std::string& str)
{
    OutputDebugStringA(str.c_str());
}
