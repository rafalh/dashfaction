#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    // Forward declarations
    struct RflRoom;
    struct AnimMesh;

    // Typedefs
    using ObjectFlags = int;
    using PhysicsFlags = int;
    using V3DType = int;
    using ObjectUse = int;

    // Object

    enum ObjectType
    {
        OT_ENTITY = 0x0,
        OT_ITEM = 0x1,
        OT_WEAPON = 0x2,
        OT_DEBRIS = 0x3,
        OT_CLUTTER = 0x4,
        OT_TRIGGER = 0x5,
        OT_EVENT = 0x6,
        OT_CORPSE = 0x7,
        OT_MOVING_GROUP = 0x8,
        OT_MOVER = 0x9,
        OT_GLARE = 0xA,
    };

    struct CollisionInfo
    {
        Vector3 hit_point;
        Vector3 normal;
        float fraction;
        int material_idx;
        int field_20;
        Vector3 obj_vel;
        int obj_handle;
        int texture;
        int field_38;
        void* face;
        int field_40;
    };
    static_assert(sizeof(CollisionInfo) == 0x44);

    struct ColSphere
    {
        Vector3 center;
        float radius;
        float field_10;
        int field_14;
    };
    static_assert(sizeof(ColSphere) == 0x18);

    struct ObjInterp
    {
        char data_[0x900];

        void Clear()
        {
            AddrCaller{0x00483330}.this_call(this);
        }
    };

    struct PhysicsInfo
    {
        float elasticity;
        float field_4;
        float friction;
        int field_C;
        float mass;
        Matrix3 field_14;
        Matrix3 field_38;
        Vector3 pos;
        Vector3 new_pos;
        Matrix3 yaw_rot_mat;
        Matrix3 new_yaw_rot_mat_unk;
        Vector3 vel;
        Vector3 rot_change_unk;
        Vector3 field_D4;
        Vector3 field_E0;
        Vector3 rot_change_unk_delta;
        float radius;
        DynamicArray<ColSphere> colliders;
        Vector3 aabb_min;
        Vector3 aabb_max;
        int flags;
        int flags2;
        float frame_time;
        CollisionInfo floor_collision_info;
    };
    static_assert(sizeof(PhysicsInfo) == 0x170);

    enum Friendliness
    {
        UNFRIENDLY = 0x0,
        NEUTRAL = 0x1,
        FRIENDLY = 0x2,
        OUTCAST = 0x3,
    };

    struct Object
    {
        RflRoom *room;
        Vector3 last_pos_in_room;
        Object *next_obj;
        Object *prev_obj;
        String name;
        int uid;
        ObjectType type;
        int team;
        int handle;
        int owner_entity_unk_handle;
        float life;
        float armor;
        Vector3 pos;
        Matrix3 orient;
        Vector3 last_pos;
        float radius;
        ObjectFlags obj_flags;
        AnimMesh *anim_mesh;
        int field_84;
        PhysicsInfo phys_info;
        Friendliness friendliness;
        int material;
        int parent_handle;
        int unk_prop_id_204;
        Vector3 field_208;
        Matrix3 mat214;
        Vector3 pos3;
        Matrix3 rot2;
        int *emitter_list_head;
        int field_26c;
        char killer_id;
        char pad[3]; // FIXME
        int multi_handle;
        ObjInterp* obj_interp;
        void* mesh_lighting_data;
        Vector3 field_280;
    };
    static_assert(sizeof(Object) == 0x28C);

    struct ObjCreateInfo
    {
        const char *v3d_filename;
        int v3d_type;
        int unk_mesh;
        float field_C;
        int material;
        float mass;
        Matrix3 unk_density_aware_mat;
        Vector3 pos;
        Matrix3 orient;
        Vector3 velocity;
        Vector3 field_78;
        float radius;
        DynamicArray<void*> col_spheres;
        int physics_flags;
    };
    static_assert(sizeof(ObjCreateInfo) == 0x98);

    static auto& ObjGetFromUid = AddrAsRef<Object*(int uid)>(0x0048A4A0);
    static auto& ObjGetFromHandle = AddrAsRef<Object*(int handle)>(0x0040A0E0);
    static auto& ObjQueueDelete = AddrAsRef<void(Object* obj)>(0x0048AB40);
}
