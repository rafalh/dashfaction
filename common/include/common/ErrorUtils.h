#pragma once

#include <windows.h>

inline std::string get_win32_error_description(DWORD error)
{
    if (error == 0)
        return "";

    LPSTR msg_buf = nullptr;
    int flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    HMODULE source = nullptr;
    if (error >= 12000 && error < 13000) {
        // most likely wininet error
        source = GetModuleHandleA("wininet.dll");
        if (source)
            flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    size_t size = FormatMessageA(flags, source, error, 0, (LPSTR)&msg_buf, 0, nullptr);
    if (size == 0)
        std::fprintf(stderr, "FormatMessageA failed: error %lu\n", GetLastError());

    std::string message(msg_buf, size);

    LocalFree(msg_buf);

    return message;
}

