#pragma once

#include "common.h"

namespace rf
{
    struct VMesh;

    enum VMeshType
    {
        MESH_TYPE_UNINITIALIZED = 0,
        MESH_TYPE_STATIC = 1,
        MESH_TYPE_CHARACTER = 2,
        MESH_TYPE_ANIM_FX = 3,
    };

    struct VMeshCollisionInput
    {
        Vector3 mesh_pos;
        Matrix3 mesh_orient;
        Vector3 start_pos;
        Vector3 dir;
        float radius;
        int flags;
        Vector3 transformed_start_pos;
        Vector3 transformed_pos_diff;

        VMeshCollisionInput()
        {
            AddrCaller{0x00416190}.this_call(this);
        }

    };
    static_assert(sizeof(VMeshCollisionInput) == 0x68);

    struct VMeshCollisionOutput
    {
        float fraction;
        Vector3 hit_point;
        Vector3 hit_normal;
        short *triangle_indices;

        VMeshCollisionOutput()
        {
            AddrCaller{0x004161D0}.this_call(this);
        }
    };
    static_assert(sizeof(VMeshCollisionOutput) == 0x20);

    static auto& VMeshGetName = AddrAsRef<const char*(VMesh* vmesh)>(0x00503470);
    static auto& VMeshGetNumCSpheres = AddrAsRef<int(VMesh *vmesh)>(0x00503250);
    static auto& VMeshGetCSphere = AddrAsRef<bool(VMesh *vmesh, int index, Vector3 *pos, float *radius)>(0x00503270);
    static auto& VMeshCollide = AddrAsRef<bool(VMesh *vmesh, VMeshCollisionInput *in, VMeshCollisionOutput *out, bool clear)>(0x005031F0);
}
