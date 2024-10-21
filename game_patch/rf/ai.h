#pragma once

#include "math/vector.h"
#include "os/array.h"
#include "os/timestamp.h"
#include "geometry.h"

namespace rf
{
    struct Entity;

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
        int num_nodes;
        GPathNode *nodes[4];
        int previous_goal;
        int current_goal;
        GPathNode start_node;
        GPathNode end_node;
        GPathNode *adjacent_node1;
        GPathNode *adjacent_node2;
        Timestamp shortcut_next_check;
        int waypoint_list_index;
        int goal_waypoint_index;
        int direction;
        int flags;
        int owner_handle;
        Timestamp resume_path_timestamp;
        Vector3 turn_towards_pos;
        int mover_wait_handle;
        int follow_style;
        Vector3 start_pos;
        int preferred_travel_ratio;
        bool going_to_next_node;
        int radius;
        Vector3 goal_pos;
        bool has_current_goal_pos;
        Vector3 current_goal_pos;
    };
    static_assert(sizeof(AiPathInfo) == 0x180);

    enum AiFlags
    {
        AIF_DEAF = 0x800,
        AIF_ALT_FIRE = 0x2000,
    };

    enum AiMode
    {
        AI_MODE_NONE = 0x0,
        AI_MODE_CATATONIC = 0x1,
        AI_MODE_WAITING = 0x2,
        AI_MODE_CHASE = 0x3,
        AI_MODE_WAYPOINTS = 0x4,
        AI_MODE_COLLECTING = 0x5,
        AI_MODE_INVESTIGATE_SOUND = 0x6,
        AI_MODE_FLEE = 0x7,
        AI_MODE_EVENT_LOOK_AT = 0x8,
        AI_MODE_EVENT_SHOOT_AT = 0x9,
        AI_MODE_FIND_PLAYER = 0xA,
        AI_MODE_MOTION_DETECT = 0xB,
        AI_MODE_FIND_COVER = 0xC,
        AI_MODE_ON_TURRET = 0xD,
        AI_MODE_HEALING = 0xE,
        AI_MODE_SECURITY_CAM = 0xF,
        AI_MODE_SET_ALARM = 0x10,
        AI_MODE_BERSERK = 0x11,
    };

    enum AiSubmode
    {
        AI_SUBMODE_NONE = 0x0,
        AI_SUBMODE_ON_PATH = 0x1,
        AI_SUBMODE_ATTACK = 0x2,
        AI_SUBMODE_ATTACK_MELEE = 0x3,
        AI_SUBMODE_COLLECTING_PATROL = 0x4,
        AI_SUBMODE_COLLECTING_PICK_UP = 0x5,
        AI_SUBMODE_COLLECTING_MOVE_TO_DROP_POINT = 0x6,
        AI_SUBMODE_COLLECTING_DROP = 0x7,
        AI_SUBMODE_MOTION_DETECT_SWEEP = 0x8,
        AI_SUBMODE_MOTION_DETECT_ATTACK = 0x9,
        AI_SUBMODE_SET_ALARM_PLAY_ANIM = 0xA,
        AI_SUBMODE_WANDER = 0xB,
        AI_SUBMODE_12 = 0xC,
    };

    enum AiAttackStyle
    {
        AI_ATTACK_STYLE_DEFAULT = 0xFFFFFFFF,
        AI_ATTACK_STYLE_EVASIVE = 0x1,
        AI_ATTACK_STYLE_STAND_GROUND = 0x2,
        AI_ATTACK_STYLE_DIRECT = 0x3,
    };

    enum AiCoverStyle
    {
        AI_COVER_STYLE_UNKNOWN = 0x0,
        AI_COVER_STYLE_CROUCH = 0x1,
        AI_COVER_STYLE_STAND = 0x2,
    };


    enum AiCooperation
    {
        AI_UNCOOPERATIVE = 0x0,
        AI_SPECIES_COOPERATIVE = 0x1,
        AI_COOPERATIVE = 0x2,
    };


    struct AiInfo
    {
        Entity *ep;
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
        Timestamp stop_continuous_weapon_timestamp;
        Timestamp delay_before_pathfind_timestamp;
        Timestamp delay_before_wander_timestamp;
        Timestamp delay_before_chase_around_corner;
        Timestamp find_cover_timestamp;
        Vector3 cover_pos;
        AiCoverStyle cover_style;
        Timestamp flee_maybe_stop_timestamp;
        Timestamp berserk_timestamp;
        Timestamp attach_rock_timestamp;
        Timestamp primary_burst_fire_next_timestamp;
        int primary_burst_fire_remaining;
        Timestamp secondary_burst_fire_next_timestamp;
        int secondary_burst_fire_remaining;
        int next_fire_secondary_slot;
        int next_fire_primary_slot;
        Timestamp unholster_done_timestamp;
        Timestamp gun_in_hand_timestamp;
        AiCooperation cooperation;
        AiMode mode;
        AiMode mode_default;
        int mode_change_time;
        int mode_parm_0;
        int mode_parm_1;
        Timestamp next_update_target_los;
        float last_seen_target_time;
        bool target_body_visible;
        bool target_eye_visible;
        bool friendly_blocking_visibility;
        Timestamp motion_detect_stop_firing;
        Timestamp motion_detect_resume_sweep;
        Timestamp motion_detect_sweep_pause;
        int motion_detect_degrees_plus;
        int motion_detect_degrees_minus;
        AiSubmode submode;
        AiSubmode submode_default;
        int submode_change_time;
        int target_handle;
        Vector3 target_acquired_pos;
        Timestamp target_acquired_timestamp;
        int look_at_handle;
        int shoot_at_handle;
        Timestamp choose_target_timestamp;
        Timestamp danger_weapon_scan_timestamp;
        int danger_weapon_handle;
        AiPathInfo current_path;
        ControlInfo ci;
        Timestamp collecting_scan_timestamp;
        Vector3 collecting_drop_pos;
        float last_damage_time;
        AiAttackStyle attack_style;
        Timestamp fire_delay_timestamp;
        int default_waypoint_path;
        int default_waypoint_path_flags;
        int on_turret_uid;
        Timestamp change_weapon_state_timestamp;
        Timestamp corpse_scan_timestamp;
        int corpse_carry_handle;
        float healing_left;
        Timestamp healing_delay_timestamp;
        int camera_uid;
        int alarm_event_uid;
        Timestamp field_4D0;
        Timestamp cower_speak_timestamp;
        Timestamp humming_timestamp;
        Timestamp resume_waypoints_timestamp;
        Timestamp player_timeout_timestamp;
        Timestamp panic_timestamp;
        Timestamp do_turning_timestamp;
        Vector3 pos_to_turn_to;
        Timestamp timer_4F8;
        Timestamp timer_4FC;
        Vector3 steering_vector;
        int vertical_offset;
        float last_turn_time;
        float last_move_time;
        float last_fire_time;
        int field_51C_;
        float movement_radius;
        float movement_height;
        bool use_custom_attack_range;
#ifdef DASH_FACTION
        bool explosive_last_damage;
#endif
        float custom_attack_range;
        int ai_flags;
    };
    static_assert(sizeof(AiInfo) == 0x534);

    static auto& ai_get_attack_range = addr_as_ref<float(AiInfo& ai)>(0x004077A0);
    static auto& ai_has_weapon = addr_as_ref<bool(AiInfo *ai_info, int weapon_type)>(0x00403250);

    static auto& ai_pause = addr_as_ref<bool>(0x005AF46D);
}
