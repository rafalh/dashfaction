#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include "os/array.h"

namespace rf
{
    struct ObjectCreateInfo;
    struct GFace;

    struct PCollisionOut
    {
        Vector3 hit_point;
        Vector3 hit_normal;
        float hit_time;
        int material;
        float inv_mass;
        Vector3 vel;
        int obj_handle;
        int bitmap_handle;
        int is_liquid;
        GFace *hit_face;
        short *hit_face_v3d;
    };
    static_assert(sizeof(PCollisionOut) == 0x44);

    struct PCollisionSphere
    {
        Vector3 center;
        float radius = 0.0f;
        float spring_const = 0.0f;
        int spring_length = 0;
    };
    static_assert(sizeof(PCollisionSphere) == 0x18);

    struct PhysicsData
    {
        float elasticity;
        float drag;
        float friction;
        int bouyancy;
        float mass;
        Matrix3 body_inv;
        Matrix3 tensor_inv;
        Vector3 pos;
        Vector3 next_pos;
        Matrix3 orient;
        Matrix3 next_orient;
        Vector3 vel;
        Vector3 rotvel;
        Vector3 ang_momentum;
        Vector3 force;
        Vector3 torque;
        float radius;
        VArray<PCollisionSphere> cspheres;
        Vector3 bbox_min;
        Vector3 bbox_max;
        int flags;
        int collision_flags;
        float frame_time_left;
        PCollisionOut collide_out;
    };
    static_assert(sizeof(PhysicsData) == 0x170);

    static auto& physics_create_object = addr_as_ref<void(PhysicsData *pd, ObjectCreateInfo *oci)>(0x0049EC90);
    static auto& physics_delete_object = addr_as_ref<void(PhysicsData *pd)>(0x0049F1D0);

    template<>
    inline void VArray<PCollisionSphere>::add(PCollisionSphere element)
    {
        AddrCaller{0x00417F30}.this_call(this, element);
    }
}
