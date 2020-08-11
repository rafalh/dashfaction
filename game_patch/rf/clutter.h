#pragma once

#include "object.h"

namespace rf
{
    struct ClutterCls;

    struct ClutterObj : Object
    {
        struct ClutterObj *next;
        struct ClutterObj *prev;
        ClutterCls* cls;
        int cls_id;
        int corpse_cls_id;
        int sound;
        Timer delete_timer;
        int delete_sound;
        int killing_dmg_type;
        Timer emitter_timer;
        Timer timer_2b4;
        AnimMesh *field_2b8;
        int field_2bC;
        DynamicArray<void*> field_2c0;
        int field_2cc;
        int use_sound;
        uint16_t killable_index;
        uint16_t field_2d6;
    };
    static_assert(sizeof(ClutterObj) == 0x2D8);

    static auto& clutter_obj_list = AddrAsRef<rf::ClutterObj>(0x005C9360);
}
