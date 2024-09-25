#pragma once

#include <windows.h>
#include <string>
#include <exception>

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

inline void print_exception(const std::exception& e, std::string& buf, int level = 0)
{
    for (int i = 0; i < level; ++i) {
        buf += ' ';
    }
    buf += e.what();
    buf += '\n';
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& e) {
        print_exception(e, buf, level + 1);
    } catch(...) {}
}

inline std::string generate_message_for_exception(const std::exception& e)
{
    std::string buf;
    print_exception(e, buf);
    return buf;
}
