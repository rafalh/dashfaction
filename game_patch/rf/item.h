#pragma once

#include "object.h"

namespace rf
{
    struct ItemInfo;

    struct Item : Object
    {
        struct Item *next;
        struct Item *prev;
        ItemInfo *info;
        int info_index;
        String name;
        int count;
        int field_2A8;
        int respawn_time;
        Timestamp respawn_timestamp;
        float alpha;
        float create_time;
        int item_flags;
        float spin_angle;
    };
    static_assert(sizeof(Item) == 0x2C4);

    static auto& item_list = addr_as_ref<Item>(0x00642DD8);
}
