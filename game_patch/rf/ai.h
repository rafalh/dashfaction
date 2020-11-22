#pragma once

#include "common.h"
#include "geometry.h"

namespace rf
{
    struct ControlInfo
    {
        Vector3 rot;
        Vector3 move;
        bool field_18;
        float mouse_dh;
        float mouse_dp;
    };

    struct AiPathInfo
    {
        int field_0;
        Vector3 *target_pos_unk_slots[4];
        int field_14;
        int current_slot;
        GPathNode field_1C;
        GPathNode field_98;
        GPathNode *field_114;
        GPathNode *field_118;
        Timestamp timer_11C;
        int waypoint_list_idx;
        int waypoint_idx;
        int field_128;
        int waypoint_method;
        int hEntityUnk;
        Timestamp timer_134;
        Vector3 orient_target;
        int moving_grp_handle_unk;
        int field_148;
        Vector3 field_14C;
        int field_158;
        char field_15C;
        int field_160;
        Vector3 unk_target_pos;
        int field_170;
        Vector3 target_pos0;
    };
    static_assert(sizeof(AiPathInfo) == 0x180);

    enum AiMode
    {
        AIM_NONE = 0x0,
        AIM_CATATONIC = 0x1,
        AIM_WAITING = 0x2,
        AIM_ATTACK = 0x3,
        AIM_WAYPOINTS = 0x4,
        AIM_COLLECTING = 0x5,
        AIM_AFTER_NOISE = 0x6,
        AIM_FLEE = 0x7,
        AIM_LOOK_AT = 0x8,
        AIM_SHOOT_AT = 0x9,
        AIM_WATCHFUL = 0xA,
        AIM_MOTION_DETECTION = 0xB,
        AIM_C = 0xC,
        AIM_TURRET_UNK = 0xD,
        AIM_HEALING = 0xE,
        AIM_CAMERA_UNK = 0xF,
        AIM_ACTIVATE_ALARM = 0x10,
        AIM_PANIC = 0x11,
    };

    enum AiAttackStyle
    {
        AIAS_DEFAULT = 0xFFFFFFFF,
        AIAS_EVASIVE = 0x1,
        AIAS_STAND_GROUND = 0x2,
        AIAS_DIRECT = 0x3,
    };

    struct AiInfo
    {
        struct Entity *entity;
        int current_primary_weapon;
        int current_secondary_weapon;
        int ammo[32];
        int clip_ammo[64];
        bool has_weapon[64];
        bool weapon_is_on[64];
        VArray<int> hate_list;
        Timestamp next_fire_primary;
        Timestamp next_fire_secondary;
        Timestamp create_weapon_delay_timestamps[2];
        bool delayed_alt_fire;
        Timestamp timer_22C;
        Timestamp watchful_mode_update_target_pos_timestamp;
        Timestamp timer_234;
        Timestamp timer_238;
        Timestamp timer_23C;
        Vector3 field_240;
        int field_24C;
        Timestamp flee_timer_unk;
        Timestamp timer_254;
        Timestamp timer_258;
        Timestamp primary_burst_delay_timestamp;
        int primary_burst_count_left;
        Timestamp seconary_burst_delay_timestamp;
        int secondary_burst_count_left;
        int field_26C;
        int field_270;
        Timestamp unholster_timestamp;
        Timestamp field_278;
        int cooperation;
        AiMode mode;
        AiMode mode_default;
        int mode_change_time;
        int mode_parm_0;
        int mode_parm_1;
        Timestamp timer_294;
        float unk_time;
        char field_29C;
        char field_29D;
        __int16 field_29E;
        Timestamp timer_2A0;
        Timestamp timer_2A4;
        Timestamp timer_2A8;
        int field_2AC;
        int field_2B0;
        int submode;
        int submode_default;
        int submode_change_time;
        int target_obj_handle;
        Vector3 target_obj_pos;
        Timestamp field_2D0;
        int look_at_handle;
        int shoot_at_handle;
        Timestamp field_2DC;
        Timestamp field_2E0;
        int field_2E4;
        AiPathInfo current_path;
        ControlInfo ci;
        Timestamp field_48C;
        Vector3 field_490;
        float last_dmg_time;
        AiAttackStyle ai_attack_style;
        Timestamp field_4A4;
        int waypoint_list_idx;
        int waypoint_method;
        int turret_uid;
        Timestamp field_4B4;
        Timestamp field_4B8;
        int held_corpse_handle;
        float field_4C0;
        Timestamp field_4C4;
        int alert_camera_uid;
        int alarm_event_uid;
        Timestamp field_4D0;
        Timestamp field_4D4;
        Timestamp field_4D8;
        Timestamp waiting_to_waypoints_mode_timestamp;
        Timestamp timeout_timestamp;
        Timestamp field_4E4;
        Timestamp field_4E8;
        Vector3 unk_target_pos;
        Timestamp timer_4F8;
        Timestamp timer_4FC;
        Vector3 pos_delta;
        int field_50C;
        float last_rot_time;
        float last_move_time;
        float last_fire_time;
        int field_51C_;
        float movement_radius;
        float field_524_;
        char use_custom_attack_range;
        float custom_attack_range;
        int flags;
    };
    static_assert(sizeof(AiInfo) == 0x534);

    static auto& AiHasWeapon = AddrAsRef<bool(AiInfo *ai_info, int weapon_type)>(0x00403250);
}
