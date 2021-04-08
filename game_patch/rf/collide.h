#pragma once

#include "math/vector.h"
#include "object.h"

namespace rf
{
    struct LevelCollisionOut
    {
        rf::Vector3 hit_point;
        float distance;
        int obj_handle;
        void* face;
    };

    static auto& collide_linesegment_level_for_multi =
        addr_as_ref<bool(rf::Vector3& p0, rf::Vector3& p1, rf::Object *ignore1, rf::Object *ignore2,
        LevelCollisionOut *out, float collision_radius, bool use_mesh_collide, float bbox_size_factor)>(0x0049C690);
}
