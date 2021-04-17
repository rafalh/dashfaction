#pragma once

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
}
