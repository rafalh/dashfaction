#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    struct Parser
    {
        bool parse_optional(const char *str)
        {
            return AddrCaller{0x005125C0}.this_call<bool>(this, str);
        }

        bool parse_required(const char *str)
        {
            return AddrCaller{0x005126A0}.this_call<bool>(this, str);
        }

        bool parse_bool()
        {
            return AddrCaller{0x005126F0}.this_call<bool>(this);
        }

        int parse_int()
        {
            return AddrCaller{0x00512750}.this_call<int>(this);
        }

        unsigned parse_uint()
        {
            return AddrCaller{0x00512840}.this_call<unsigned>(this);
        }

        float parse_float()
        {
            return AddrCaller{0x00512920}.this_call<float>(this);
        }

        int parse_string(String *out_str, char first_char = '"', char last_char = '"')
        {
            return AddrCaller{0x00512BB0}.this_call<int>(this, out_str, first_char, last_char);
        }

        bool parse_eof() const
        {
            return AddrCaller{0x00513780}.this_call<bool>(this);
        }

        void error(const char* msg)
        {
            AddrCaller{0x00512370}.this_call(this, msg);
        }
    };
}
