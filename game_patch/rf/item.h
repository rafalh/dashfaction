#pragma once

#include "object.h"

namespace rf
{
    struct ItemCls;

    struct ItemObj : Object
    {
        struct ItemObj *next;
        struct ItemObj *prev;
        ItemCls *cls;
        int cls_id;
        String script_name;
        int count;
        int field_2A8;
        int respawn_time;
        Timer respawn_timer;
        float alpha;
        float create_time;
        int item_flags;
        float spin_angle;
    };
    static_assert(sizeof(ItemObj) == 0x2C4);

    static auto& item_obj_list = AddrAsRef<rf::ItemObj>(0x00642DD8);
}
