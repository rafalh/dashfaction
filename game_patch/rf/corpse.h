#pragma once

#include "object.h"
#include "entity.h"

namespace rf
{
    struct Corpse : Object
    {
        Corpse *next;
        Corpse *prev;
        float create_time;
        float lifetime_seconds;
        int corpse_flags;
        int entity_type;
        String corpse_pose_name;
        Timestamp emitter_kill_timestamp;
        float body_temp;
        int corpse_state_vmesh_anim_index;
        int corpse_action_vmesh_anim_index;
        int corpse_drop_vmesh_anim_index;
        int corpse_carry_vmesh_anim_index;
        int corpse_pose;
        VMesh *helmet_v3d_handle;
        int item_handle;
        EntityFireInfo *burn_info;
        int field_2D4;
        Color ambient_color;
        ShadowInfo shadow_info;
    };
    static_assert(sizeof(Corpse) == 0x318);

    static auto& corpse_from_handle = addr_as_ref<Corpse*(int handle)>(0x004174C0);

    static auto& corpse_list = addr_as_ref<Corpse>(0x005CABB8);
}
