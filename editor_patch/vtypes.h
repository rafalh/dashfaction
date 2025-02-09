#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    struct String
    {
        int max_len;
        char* buf;

        operator const char*() const
        {
            return buf;
        }
    };

    struct File
    {
        enum SeekOrigin {
            seek_set = 0,
            seek_cur = 1,
            seek_end = 2,
        };

        [[nodiscard]] bool check_version(int min_ver) const
        {
            return AddrCaller{0x004CF650}.this_call<bool>(this, min_ver);
        }

        [[nodiscard]] int error() const
        {
            return AddrCaller{0x004D01F0}.this_call<bool>(this);
        }

        int seek(int pos, SeekOrigin origin)
        {
            return AddrCaller{0x004D00C0}.this_call<int>(this, pos, origin);
        }

        int read(void *buf, std::size_t buf_len, int min_ver = 0, int unused = 0)
        {
            return AddrCaller{0x004D0F40}.this_call<int>(this, buf, buf_len, min_ver, unused);
        }

        template<typename T>
        T read(int min_ver = 0, T def_val = 0)
        {
            if (check_version(min_ver)) {
                T val;
                read(&val, sizeof(val));
                if (!error()) {
                    return val;
                }
            }
            return def_val;
        }

        void write(const void *data, std::size_t data_len)
        {
            return AddrCaller{0x004D13F0}.this_call(this, data, data_len);
        }

        template<typename T>
        void write(T value)
        {
            write(static_cast<void*>(&value), sizeof(value));
        }
    };
}
