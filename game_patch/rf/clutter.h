#pragma once

#include "object.h"
#include "os/timestamp.h"

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
        int sound_handle;
        Timestamp delayed_kill_timestamp;
        int delayed_kill_sound;
        int dmg_type_that_killed_me;
        Timestamp emitter_kill_timestamp;
        Timestamp corpse_create_timestamp;
        VMesh *corpse_vmesh_handle;
        int current_skin_index;
        VArray<int> links;
        bool already_spawned_glass;
        int use_sound;
        uint16_t killable_index;
    };
    static_assert(sizeof(Clutter) == 0x2D8);

    enum MonitorState
    {
        MONITOR_STATE_ON = 0x0,
        MONITOR_STATE_OFF = 0x1,
        MONITOR_STATE_STATIC = 0x2,
    };


    struct Monitor
    {
        Monitor *next;
        Monitor *prev;
        int state;
        int clutter_handle;
        int camera_handle;
        int user_bitmap;
        VArray<int> camera_list;
        Timestamp next_cam_switch;
        float cycle_delay_seconds;
        int resolution_index;
        int bm_width;
        int bm_height;
        int flags;
    };
    static_assert(sizeof(Monitor) == 0x3C);

    // Monitor flags
    enum MonitorFlags
    {
        MF_OBJ_RENDERED = 0x2,
        MF_BM_RENDERED = 0x4,
        MF_CONTINOUSLY_UPDATE = 0x8,
    };

    static auto& clutter_list = addr_as_ref<Clutter>(0x005C9360);
    static auto& monitor_list = addr_as_ref<Monitor>(0x005C98A8);

    static auto& clutter_restore_mesh = addr_as_ref<void(Clutter *clutter, const char *mesh_name)>(0x00410ED0);
}
