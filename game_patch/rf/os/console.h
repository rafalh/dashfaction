#pragma once

#include <patch_common/MemUtils.h>
#include <memory>
#include <common/utils/string-utils.h>
#include "string.h"
#include "../gr/gr.h"

namespace rf
{
    typedef void(*ConsoleCommandFuncPtr)();

    struct ConsoleCommand
    {
        const char *name;
        const char *help;
        ConsoleCommandFuncPtr func;

        inline static const auto init =
            reinterpret_cast<void(__thiscall*)(ConsoleCommand* this_, const char* name, const char* help, ConsoleCommandFuncPtr func)>(
                0x00509A70);
    };
    static_assert(sizeof(ConsoleCommand) == 0xC);

    enum DcArgType
    {
        CONSOLE_ARG_NONE = 0x0001,
        CONSOLE_ARG_STR = 0x0002,
        CONSOLE_ARG_INT = 0x0008,
        CONSOLE_ARG_FLOAT = 0x0010,
        CONSOLE_ARG_HEX = 0x0020,
        CONSOLE_ARG_TRUE = 0x0040,
        CONSOLE_ARG_FALSE = 0x0080,
        CONSOLE_ARG_PLUS = 0x0100,
        CONSOLE_ARG_MINUS = 0x0200,
        CONSOLE_ARG_COMMA = 0x0400,
        CONSOLE_ARG_ANY = 0xFFFFFFFF,
    };

    static auto& console_output = addr_as_ref<void(const char* text, const Color* color)>(0x00509EC0);
    static auto& console_get_arg = addr_as_ref<void(unsigned type, bool preserve_case)>(0x0050AED0);

    // Note: console_printf is reimplemented to allow static validation of the format string
    PRINTF_FMT_ATTRIBUTE(1, 2)
    inline void console_printf(const char* format, ...)
    {
        String str;
        va_list args;
        va_start(args, format);
        int size = std::vsnprintf(nullptr, 0, format, args) + 1;
        va_end(args);
        std::unique_ptr<char[]> buf(new char[size]);
        va_start(args, format);
        std::vsnprintf(buf.get(), size, format, args);
        va_end(args);
        console_output(buf.get(), nullptr);
    }

    //static auto& console_commands = addr_as_ref<ConsoleCommand*[30]>(0x01775530);
    static auto& console_num_commands = addr_as_ref<int>(0x0177567C);

    static auto& console_run = addr_as_ref<int>(0x01775110);
    static auto& console_help = addr_as_ref<int>(0x01775224);
    static auto& console_status = addr_as_ref<int>(0x01774D00);
    static auto& console_arg_type = addr_as_ref<unsigned>(0x01774D04);
    static auto& console_str_arg = addr_as_ref<char[256]>(0x0175462C);
    static auto& console_int_arg = addr_as_ref<int>(0x01775220);
    static auto& console_float_arg = addr_as_ref<float>(0x01754628);
    static auto& console_cmd_line = addr_as_ref<char[256]>(0x01775330);
    static auto& console_cmd_line_len = addr_as_ref<int>(0x0177568C);
}
