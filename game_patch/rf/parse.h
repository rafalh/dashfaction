#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    struct StrParser
    {
        bool OptionalString(const char *str)
        {
            return AddrCaller{0x005125C0}.this_call<bool>(this, str);
        }

        bool RequiredString(const char *str)
        {
            return AddrCaller{0x005126A0}.this_call<bool>(this, str);
        }

        bool GetBool()
        {
            return AddrCaller{0x005126F0}.this_call<bool>(this);
        }

        int GetInt()
        {
            return AddrCaller{0x00512750}.this_call<int>(this);
        }

        unsigned GetUInt()
        {
            return AddrCaller{0x00512840}.this_call<unsigned>(this);
        }

        float GetFloat()
        {
            return AddrCaller{0x00512920}.this_call<float>(this);
        }

        int GetString(String *out_str, char first_char = '"', char last_char = '"')
        {
            return AddrCaller{0x00512BB0}.this_call<int>(this, out_str, first_char, last_char);
        }

        bool IsEof() const
        {
            return AddrCaller{0x00513780}.this_call<bool>(this);
        }

        void Error(const char* msg)
        {
            AddrCaller{0x00512370}.this_call(this, msg);
        }
    };
}
