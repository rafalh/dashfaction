#pragma once

#include "object.h"

namespace rf
{
    struct ClutterInfo;

    struct Clutter : Object
    {
        struct Clutter *next;
        struct Clutter *prev;
        ClutterInfo* info;
        int info_index;
        int corpse_index;
        int sound;
        Timestamp delete_timestamp;
        int delete_sound;
        int killing_dmg_type;
        Timestamp emitter_timestamp;
        Timestamp timer_2b4;
        VMesh *field_2b8;
        int field_2bC;
        VArray<void*> field_2c0;
        int field_2cc;
        int use_sound;
        uint16_t killable_index;
        uint16_t field_2d6;
    };
    static_assert(sizeof(Clutter) == 0x2D8);

    static auto& clutter_list = addr_as_ref<Clutter>(0x005C9360);
}
