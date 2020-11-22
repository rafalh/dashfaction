#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"
#include "../utils/enum-bitwise-operators.h"

namespace rf
{
    // Forward declarations
    struct GRoom;
    struct GSolid;
    struct VMesh;

    // Typedefs
    using PhysicsFlags = int;
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
        OT_MOVER = 0x8,
        OT_MOVER_BRUSH = 0x9,
        OT_GLARE = 0xA,
    };

    enum ObjectFlags
    {
        OF_WAS_RENDERED = 0x10,
        OF_HAS_ALPHA = 0x100000,
    };

    struct PCollisionOut
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
    static_assert(sizeof(PCollisionOut) == 0x44);

    struct PCollisionSphere
    {
        Vector3 center;
        float radius = 0.0f;
        float spring_const = 0.0f;
        int spring_length = 0;
    };
    static_assert(sizeof(PCollisionSphere) == 0x18);

    struct ObjInterp
    {
        char data_[0x900];

        void Clear()
        {
            AddrCaller{0x00483330}.this_call(this);
        }
    };

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
        Vector3 field_D4;
        Vector3 field_E0;
        Vector3 rot_change_unk_delta;
        float radius;
        VArray<PCollisionSphere> cspheres;
        Vector3 bbox_min;
        Vector3 bbox_max;
        int flags;
        int flags2;
        float frame_time;
        PCollisionOut collide_out;
    };
    static_assert(sizeof(PhysicsData) == 0x170);

    enum Friendliness
    {
        UNFRIENDLY = 0x0,
        NEUTRAL = 0x1,
        FRIENDLY = 0x2,
        OUTCAST = 0x3,
    };

    struct Object
    {
        GRoom *room;
        Vector3 correct_pos;
        Object *next_obj;
        Object *prev_obj;
        String name;
        int uid;
        ObjectType type;
        int team;
        int handle;
        int parent_handle;
        float life;
        float armor;
        Vector3 pos;
        Matrix3 orient;
        Vector3 last_pos;
        float radius;
        ObjectFlags obj_flags;
        VMesh *vmesh;
        int vmesh_submesh;
        PhysicsData p_data;
        Friendliness friendliness;
        int material;
        int host_handle;
        int host_tag_handle;
        Vector3 host_offset;
        Matrix3 host_orient;
        Vector3 start_pos;
        Matrix3 start_orient;
        int *emitter_list_head;
        int root_bone_index;
        char killer_netid;
        int server_handle;
        ObjInterp* obj_interp;
        void* mesh_lighting_data;
        Vector3 relative_transition_pos;
    };
    static_assert(sizeof(Object) == 0x28C);

    enum VMeshType
    {
        MESH_TYPE_UNINITIALIZED = 0,
        MESH_TYPE_STATIC = 1,
        MESH_TYPE_CHARACTER = 2,
        MESH_TYPE_ANIM_FX = 3,
    };

    struct ObjectCreateInfo
    {
        const char* v3d_filename = nullptr;
        VMeshType v3d_type = MESH_TYPE_UNINITIALIZED;
        GSolid* solid = nullptr;
        float drag = 0.0f;
        int material = 0;
        float mass = 0.0f;
        Matrix3 body_inv;
        Vector3 pos;
        Matrix3 orient;
        Vector3 vel;
        Vector3 rotvel;
        float radius = 0.0f;
        VArray<PCollisionSphere> spheres;
        int physics_flags = 0;
    };
    static_assert(sizeof(ObjectCreateInfo) == 0x98);


    static auto& ObjLookupFromUid = AddrAsRef<Object*(int uid)>(0x0048A4A0);
    static auto& ObjFromHandle = AddrAsRef<Object*(int handle)>(0x0040A0E0);
    static auto& ObjQueueDelete = AddrAsRef<void(Object* obj)>(0x0048AB40);
    static auto& ObjFindRootBonePos = AddrAsRef<void(Object*, Vector3&)>(0x0048AC70);
    static auto& PhysicsCreateObject = AddrAsRef<void (rf::PhysicsData *pd, rf::ObjectCreateInfo *oci)>(0x0049EC90);
    static auto& PhysicsDeleteObject = AddrAsRef<void (rf::PhysicsData *pd)>(0x0049F1D0);
    static auto& VMeshGetNumCSpheres = AddrAsRef<int(rf::VMesh *vmesh)>(0x00503250);
    static auto& VMeshGetCSphere = AddrAsRef<bool(rf::VMesh *vmesh, int index, rf::Vector3 *pos, float *radius)>(0x00503270);
    static auto& VArray_PCollisionSphere__Add = AddrAsRef<int __thiscall(rf::VArray<rf::PCollisionSphere> *this_, rf::PCollisionSphere csphere)>(0x00417F30);

}

template<>
struct EnableEnumBitwiseOperators<rf::ObjectFlags> : std::true_type {};
