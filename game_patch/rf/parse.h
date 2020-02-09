#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    struct StrParser
    {
        bool OptionalString(const char *str)
        {
            return reinterpret_cast<bool(__thiscall*)(StrParser*, const char*)>(0x005125C0)(this, str);
        }

        bool RequiredString(const char *str)
        {
            return reinterpret_cast<bool(__thiscall*)(StrParser*, const char*)>(0x005126A0)(this, str);
        }

        bool GetBool()
        {
            return reinterpret_cast<bool(__thiscall*)(StrParser*)>(0x005126F0)(this);
        }

        int GetInt()
        {
            return reinterpret_cast<int(__thiscall*)(StrParser*)>(0x00512750)(this);
        }

        unsigned GetUInt()
        {
            return reinterpret_cast<int(__thiscall*)(StrParser*)>(0x00512840)(this);
        }

        float GetFloat()
        {
            return reinterpret_cast<float(__thiscall*)(StrParser*)>(0x00512920)(this);
        }

        int GetString(String *out_str, char first_char = '"', char last_char = '"')
        {
            return reinterpret_cast<int(__thiscall*)(StrParser*, String*, char, char)>(0x00512BB0)
                (this, out_str, first_char, last_char);
        }

        bool IsEof() const
        {
            return reinterpret_cast<bool(__thiscall*)(const StrParser*)>(0x00513780)(this);
        }

        void Error(const char* msg)
        {
            reinterpret_cast<void(__thiscall*)(StrParser*, const char*)>(0x00512370)(this, msg);
        }
    };
}
