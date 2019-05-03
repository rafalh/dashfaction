#pragma once

#include <windows.h>

inline std::string GetWin32ErrorDescription(DWORD error)
{
    if (error == 0)
        return "";

    LPSTR msg_buf = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg_buf, 0, nullptr);

    std::string message(msg_buf, size);

    LocalFree(msg_buf);

    return message;
}

