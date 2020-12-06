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

    struct Monitor
    {
        struct Monitor *next;
        struct Monitor *prev;
        int monitor_state;
        int clutter_handle;
        int current_camera_handle;
        int bitmap;
        VArray<int> cameras_handles_array;
        Timestamp camera_cycle_timestamp;
        float cycle_delay;
        int gap2C;
        int width;
        int height;
        int flags;
    };
    static_assert(sizeof(Monitor) == 0x3C);

    // Monitor flags
    constexpr int MF_BM_RENDERED = 4;

    static auto& clutter_list = addr_as_ref<Clutter>(0x005C9360);
    static auto& monitor_list = addr_as_ref<Monitor>(0x005C98A8);
}
