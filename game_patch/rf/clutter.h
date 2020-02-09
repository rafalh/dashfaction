#pragma once

#include "object.h"

namespace rf
{
    struct ClutterObj
    {
        Object _super;
        struct ClutterObj *next;
        struct ClutterObj *prev;
        void* cls;
        int cls_id;
        int corpse_cls_id;
        int sound;
        Timer timer_2a4;
        int snd_2a8;
        int killing_dmg_type;
        Timer emitter_timer;
        Timer timer_2b4;
        AnimMesh *field_2b8;
        int field_2bC;
        DynamicArray<void*> field_2c0;
        int field_2cc;
        int field_2d0;
        uint16_t killable_index;
        uint16_t field_2d6;
    };
    static_assert(sizeof(ClutterObj) == 0x2D8);
}
