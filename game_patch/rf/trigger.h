#pragma once

#include "object.h"

namespace rf
{
    struct TriggerObj
    {
        Object _super;
        TriggerObj *next;
        TriggerObj *prev;
        int shape;
        Timer resets_after_timer;
        int resets_after_ms;
        int resets_counter;
        int resets_times;
        float last_activation_time;
        int key_item_cls_id;
        int flags;
        String field_2b4;
        int field_2bc;
        int airlock_room_uid;
        int activated_by;
        Vector3 box_size;
        DynamicArray<> links;
        Timer activation_failed_timer;
        int activation_failed_entity_handle;
        Timer field_2e8;
        char one_way;
        float button_active_time_seconds;
        Timer field_2f4;
        float inside_time_seconds;
        Timer inside_timer;
        int attached_to_uid;
        int use_clutter_uid;
        char team;
    };
    static_assert(sizeof(TriggerObj) == 0x30C);
}
