#pragma once

#include "object.h"
#include "os/timestamp.h"

namespace rf
{
    struct ItemInfo
    {
        String cls_name;
        String v3d_filename;
        const char* hud_msg_name;
        int pickup_msg_single;
        int pickup_msg_multi;
        int pickup_msg_wa_single;
        int pickup_msg_wa_multi;
        int v3d_type;
        int pickup_snd;
        int count;
        int count_single;
        int count_multi;
        int respawn_time_millis;
        int ammo_for_weapon_id;
        int gives_weapon_id;
        int flags; // see ItemInfoFlags
        int(__cdecl* touch_callback)(int entity_handle, int item_handle, int count);
        int use_callback_not_sure;
    };
    static_assert(sizeof(ItemInfo) == 0x50);

    enum ItemInfoFlags
    {
        IIF_NO_PICKUP = 0x1,
        IIF_SPINS_IN_MULTI = 0x2,
    };

    struct Item : Object
    {
        struct Item* next;
        struct Item* prev;
        ItemInfo* info;
        int info_index;
        String name;
        int count;
        int field_2A8;
        int respawn_time_ms;
        Timestamp respawn_next;
        float alpha;
        float create_time;
        int item_flags;
        float spin_angle;
    };
    static_assert(sizeof(Item) == 0x2C4);

    static auto& item_list = addr_as_ref<Item>(0x00642DD8);
    static auto& item_info = addr_as_ref<ItemInfo[96]>(0x006430A0);
    static auto& num_item_types = addr_as_ref<int>(0x00644EA0);

    static auto& item_lookup_type = addr_as_ref<int(const char* name)>(0x00459430);
    static auto& item_restore_mesh = addr_as_ref<void(Item* item, const char* mesh_name)>(0x00459BB0);
}
