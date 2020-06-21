#pragma once

#include <patch_common/MemUtils.h>
#include <memory>
#include "../utils/string-utils.h"
#include "common.h"

namespace rf
{
    typedef void(*DcCmdHandler)();

    struct DcCommand
    {
        const char *cmd_name;
        const char *descr;
        void(*func)();

        inline static const auto Init =
            reinterpret_cast<void(__thiscall*)(rf::DcCommand* self, const char* cmd, const char* descr, DcCmdHandler handler)>(
                0x00509A70);
    };
    static_assert(sizeof(DcCommand) == 0xC);

    enum DcArgType
    {
        DC_ARG_NONE = 0x0001,
        DC_ARG_STR = 0x0002,
        DC_ARG_INT = 0x0008,
        DC_ARG_FLOAT = 0x0010,
        DC_ARG_HEX = 0x0020,
        DC_ARG_TRUE = 0x0040,
        DC_ARG_FALSE = 0x0080,
        DC_ARG_PLUS = 0x0100,
        DC_ARG_MINUS = 0x0200,
        DC_ARG_COMMA = 0x0400,
        DC_ARG_ANY = 0xFFFFFFFF,
    };

    static auto& DcPrint = AddrAsRef<void(const char *text, const uint32_t *color)>(0x00509EC0);
    static auto& DcGetArg = AddrAsRef<void(int type, bool preserve_case)>(0x0050AED0);
    static auto& DcRunCmd = AddrAsRef<int(const char *cmd)>(0x00509B00);

    // Note: DcPrintf is reimplemented to allow static validation of format string
    PRINTF_FMT_ATTRIBUTE(1, 2)
    inline void DcPrintf(const char* format, ...)
    {
        String str;
        va_list args;
        va_start(args, format);
        int size = vsnprintf(nullptr, 0, format, args) + 1;
        va_end(args);
        std::unique_ptr<char[]> buf(new char[size]);
        va_start(args, format);
        vsnprintf(buf.get(), size, format, args);
        va_end(args);
        DcPrint(buf.get(), nullptr);
    }

    //static auto& DcCommands = AddrAsRef<DcCommand*[30]>(0x01775530);
    static auto& dc_num_commands = AddrAsRef<uint32_t>(0x0177567C);

    static auto& dc_run = AddrAsRef<uint32_t>(0x01775110);
    static auto& dc_help = AddrAsRef<uint32_t>(0x01775224);
    static auto& dc_status = AddrAsRef<uint32_t>(0x01774D00);
    static auto& dc_arg_type = AddrAsRef<uint32_t>(0x01774D04);
    static auto& dc_str_arg = AddrAsRef<char[256]>(0x0175462C);
    static auto& dc_int_arg = AddrAsRef<int>(0x01775220);
    static auto& dc_float_arg = AddrAsRef<float>(0x01754628);
    static auto& dc_cmd_line = AddrAsRef<char[256]>(0x01775330);
    static auto& dc_cmd_line_len = AddrAsRef<int>(0x0177568C);
}
