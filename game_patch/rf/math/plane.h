#pragma once

#include "vector.h"

namespace rf
{
    struct Plane
    {
        Vector3 normal;
        float offset;
    };
    static_assert(sizeof(Plane) == 0x10);
}
