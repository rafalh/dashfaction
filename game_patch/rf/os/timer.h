#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    static auto& timer_get = addr_as_ref<int(int frequency)>(0x00504AB0);

    inline int timer_get_microseconds()
    {
        return timer_get(1000000);
    }

    inline int timer_get_milliseconds()
    {
        return timer_get(1000);
    }

    inline int timer_get_seconds()
    {
        return timer_get(1);
    }
}
