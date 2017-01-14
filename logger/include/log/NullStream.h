#pragma once

namespace logging
{
    class NullStream {};

    template <typename T> NullStream &operator<<(NullStream &s, const T &)
    {
        return s;
    }
}
