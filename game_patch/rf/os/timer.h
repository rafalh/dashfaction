#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    static auto& timer_get = addr_as_ref<int(int frequency)>(0x00504AB0);
    static auto& timer_add_delta_time = addr_as_ref<int(int delta_ms)>(0x004FA2D0);

    static auto& timer_base = addr_as_ref<int64_t>(0x01751BF8);
    static auto& timer_last_value = addr_as_ref<int64_t>(0x01751BD0);
    static auto& timer_freq = addr_as_ref<int32_t>(0x01751C04);

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
