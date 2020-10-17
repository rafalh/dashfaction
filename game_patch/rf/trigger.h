#pragma once

#include "object.h"

namespace rf
{
    struct Trigger : Object
    {
        Trigger *next;
        Trigger *prev;
        int shape;
        Timestamp resets_after_timestamp;
        int resets_after_ms;
        int resets_counter;
        int resets_times;
        float last_activation_time;
        int key_item_cls_id;
        int trigger_flags;
        String field_2b4;
        int field_2bc;
        int airlock_room_uid;
        int activated_by;
        Vector3 box_size;
        VArray<int> links;
        Timestamp activation_failed_timestamp;
        int activation_failed_entity_handle;
        Timestamp field_2e8;
        char one_way;
        float button_active_time_seconds;
        Timestamp field_2f4;
        float inside_time_seconds;
        Timestamp inside_timestamp;
        int attached_to_uid;
        int use_clutter_uid;
        char team;
    };
    static_assert(sizeof(Trigger) == 0x30C);
}
