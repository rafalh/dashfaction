#pragma once

#include "vector.h"

namespace rf
{
    // Intersection helpers
    using IxLineSegmentBoundingBoxType =
        bool(const Vector3& box_min, const Vector3& box_max, const Vector3& p0, const Vector3& p1, Vector3* hit_pt);
    static auto& ix_linesegment_boundingbox = addr_as_ref<IxLineSegmentBoundingBoxType>(0x00508B70);
    static auto& ix_point_in_box =
        addr_as_ref<bool(const Vector3& pnt, const Vector3& mn, const Vector3& mx)>(0x005079F0);
}
