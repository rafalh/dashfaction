#pragma once

#include <common/utils/string-utils.h>
#include <patch_common/MemUtils.h>
#include <memory>
#include <format>
#include <cstdarg>
#include "string.h" // NOLINT(modernize-deprecated-headers)
#include "../gr/gr.h"

namespace rf::console
{
    using CommandFuncPtr = void(*)();

    struct Command
    {
        const char *name;
        const char *help;
        CommandFuncPtr func;

        inline static const auto init =
            reinterpret_cast<void(__thiscall*)(Command* this_, const char* name, const char* help, CommandFuncPtr func)>(
                0x00509A70);
    };
    static_assert(sizeof(Command) == 0xC);

    enum ArgType
    {
        ARG_NONE = 0x0001,
        ARG_STR = 0x0002,
        ARG_INT = 0x0008,
        ARG_FLOAT = 0x0010,
        ARG_HEX = 0x0020,
        ARG_TRUE = 0x0040,
        ARG_FALSE = 0x0080,
        ARG_PLUS = 0x0100,
        ARG_MINUS = 0x0200,
        ARG_COMMA = 0x0400,
        ARG_ANY = 0xFFFFFFFF,
    };

    static auto& output = addr_as_ref<void(const char* text, const Color* color)>(0x00509EC0);
    static auto& get_arg = addr_as_ref<void(unsigned type, bool preserve_case)>(0x0050AED0);

    // Note: console_printf is reimplemented to allow static validation of the format string
    PRINTF_FMT_ATTRIBUTE(1, 2)
    inline void printf(const char* format, ...)
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
        output(buf.get(), nullptr);
    }

    template<typename... Args>
    inline void print(std::format_string<Args...> fmt, Args&&... args)
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        output(s.c_str(), nullptr);
    }

    //static auto& commands = addr_as_ref<ConsoleCommand*[30]>(0x01775530);
    static auto& num_commands = addr_as_ref<int>(0x0177567C);

    static auto& run = addr_as_ref<int>(0x01775110);
    static auto& help = addr_as_ref<int>(0x01775224);
    static auto& status = addr_as_ref<int>(0x01774D00);
    static auto& arg_type = addr_as_ref<unsigned>(0x01774D04);
    static auto& str_arg = addr_as_ref<char[256]>(0x0175462C);
    static auto& int_arg = addr_as_ref<int>(0x01775220);
    static auto& float_arg = addr_as_ref<float>(0x01754628);
    static auto& cmd_line = addr_as_ref<char[256]>(0x01775330);
    static auto& cmd_line_len = addr_as_ref<int>(0x0177568C);
    static auto& history_current_index = addr_as_ref<int>(0x01775690);
    static auto& history = addr_as_ref<char[8][256]>(0x017744F4);
    static auto& history_max_index = addr_as_ref<int>(0x005A4030);
}
