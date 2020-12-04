#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"
#include "physics.h"
#include "vmesh.h"
#include "../utils/enum-bitwise-operators.h"

namespace rf
{
    // Forward declarations
    struct GRoom;
    struct GSolid;
    struct VMesh;

    // Typedefs
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
        OF_DELAYED_DELETE = 0x2,
        OF_WAS_RENDERED = 0x10,
        OF_IN_LIQUID = 0x80000,
        OF_HAS_ALPHA = 0x100000,
    };

    struct ObjInterp
    {
        char data_[0x900];

        void Clear()
        {
            AddrCaller{0x00483330}.this_call(this);
        }
    };

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

    static auto& obj_lookup_from_uid = addr_as_ref<Object*(int uid)>(0x0048A4A0);
    static auto& obj_from_handle = addr_as_ref<Object*(int handle)>(0x0040A0E0);
    static auto& obj_flag_dead = addr_as_ref<void(Object* obj)>(0x0048AB40);
    static auto& obj_find_root_bone_pos = addr_as_ref<void(Object*, Vector3&)>(0x0048AC70);
    static auto& obj_update_liquid_status = addr_as_ref<void(Object* obj)>(0x00486C30);

    static auto& object_list = addr_as_ref<Object>(0x0073D880);
}

template<>
struct EnableEnumBitwiseOperators<rf::ObjectFlags> : std::true_type {};
