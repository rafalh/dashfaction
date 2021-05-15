#pragma once

#include "vector.h"

namespace rf
{
    struct Plane
    {
        Vector3 normal;
        float offset;

        [[nodiscard]] float distance_to_point(const Vector3& pt) const
        {
            return pt.dot_prod(normal) + offset;
        }
    };
    static_assert(sizeof(Plane) == 0x10);
}
