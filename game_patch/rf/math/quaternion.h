#pragma once

namespace rf
{
    struct Quaternion
    {
        float x, y, z, w;
    };
    static_assert(sizeof(Quaternion) == 0x10);
}
