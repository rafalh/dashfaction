#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    // Forward declarations
    struct Entity;
    struct VMesh;
    struct Player;
    struct PlayerNetData;
    struct AiInfo;

    /* Camera */

    enum CameraMode
    {
        CAM_FIRST_PERSON = 0x0,
        CAM_THIRD_PERSON = 0x1,
        CAM_FREELOOK = 0x2,
        CAM_3 = 0x3,
        CAM_FREELOOK2 = 0x4,
        CAM_CUTSCENE_UNK = 0x5,
        CAM_FIXED = 0x6,
        CAM_ORBIT = 0x7,
    };

    struct Camera
    {
        Entity *camera_entity;
        Player *player;
        CameraMode mode;
    };
    static_assert(sizeof(Camera) == 0xC);

    static auto& camera_set_first_person = addr_as_ref<bool(Camera *camera)>(0x0040DDF0);
    static auto& camera_set_freelook = addr_as_ref<bool(Camera *camera)>(0x0040DCF0);
    static auto& camera_set_fixed = addr_as_ref<bool(Camera *camera)>(0x0040DF70);
    static auto& camera_get_pos = addr_as_ref<void(Vector3* camera_pos, Camera *camera)>(0x0040D760);
    static auto& camera_get_orient = addr_as_ref<void(Matrix3* camera_orient, Camera *camera)>(0x0040D780);

    /* Config */

    struct ControlConfigItem
    {
        int16_t default_scan_codes[2];
        int16_t default_mouse_btn_id;
        int16_t field_6;
        int field_8;
        String name;
        int16_t scan_codes[2];
        int16_t mouse_btn_id;
        int16_t field_1a;
    };
    static_assert(sizeof(ControlConfigItem) == 0x1C);

    struct ControlConfig
    {
        float mouse_sensitivity;
        bool mouse_look;
        int field_ec;
        ControlConfigItem keys[128];
        int ctrl_count;
        int field_ef4;
        int field_ef8;
        int field_efc;
        int field_f00;
        char field_f04;
        char mouse_y_invert;
        char field_e22;
        char field_e23;
        int field_f08;
        int field_f0c;
        int field_f10;
        int field_f14;
        int field_f18;
        int field_f1c;
        int field_f20;
        int field_f24;
        int field_f28;
    };
    static_assert(sizeof(ControlConfig) == 0xE48);

    struct PlayerSettings
    {
        ControlConfig controls;
        int field_f2c;
        int field_f30;
        bool field_f34;
        bool field_e51;
        bool render_fpgun;
        bool weapons_sway;
        bool toggle_crouch;
        bool field_f39;
        bool field_f3a;
        bool show_hud;
        bool show_hud_ammo;
        bool show_hud_status;
        bool show_hud_messages;
        bool show_crosshair;
        bool autoswitch_weapons;
        bool dont_autoswitch_to_explosives;
        bool shadows_enabled;
        bool decals_enabled;
        bool dynamic_lightining_enabled;
        bool field_f45;
        bool field_f46;
        bool field_f47;
        bool filtering_level;
        int detail_level;
        int textures_resolution_level;
        int character_detail_level;
        float field_f58;
        int multi_character;
        char name[12];
    };
    static_assert(sizeof(PlayerSettings) == 0xE88);

    enum ControlAction
    {
        CA_PRIMARY_ATTACK = 0x0,
        CA_SECONDARY_ATTACK = 0x1,
        CA_USE = 0x2,
        CA_JUMP = 0x3,
        CA_CROUCH = 0x4,
        CA_HIDE_WEAPON = 0x5,
        CA_RELOAD = 0x6,
        CA_NEXT_WEAPON = 0x7,
        CA_PREV_WEAPON = 0x8,
        CA_CHAT = 0x9,
        CA_TEAM_CHAT = 0xA,
        CA_FORWARD = 0xB,
        CA_BACKWARD = 0xC,
        CA_SLIDE_LEFT = 0xD,
        CA_SLIDE_RIGHT = 0xE,
        CA_SLIDE_UP = 0xF,
        CA_SLIDE_DOWN = 0x10,
        CA_LOOK_DOWN = 0x11,
        CA_LOOK_UP = 0x12,
        CA_TURN_LEFT = 0x13,
        CA_TURN_RIGHT = 0x14,
        CA_MESSAGES = 0x15,
        CA_MP_STATS = 0x16,
        CA_QUICK_SAVE = 0x17,
        CA_QUICK_LOAD = 0x18,
    };

    /* Player */

    struct PlayerLevelStats
    {
        uint16_t field_0;
        int16_t score;
        int16_t caps;
        char padding[2];
    };
    static_assert(sizeof(PlayerLevelStats) == 0x8);

    struct PlayerFpgunData
    {
        int pending_weapon;
        Timestamp draw_delay_timestamp;
        Timestamp spark_timestamp;
        bool skip_next_fpgun_render;
        Timestamp decremental_ammo_update_timestamp;
        bool zooming_in;
        bool zooming_out;
        float zoom_factor;
        bool locking_on_target;
        bool locked_on_target;
        int locked_target_handle;
        Timestamp locking_time;
        Timestamp rescan_time;
        Timestamp rocket_scan_timer_unk1;
        bool scanning_for_target;
        bool drawing_entity_bmp;
        Color rail_gun_entity_color;
        int rail_gun_fork_tag;
        int rail_gun_reload_tag;
        float time_elapsed_since_firing;
        Vector3 old_cam_pos;
        Matrix3 old_cam_orient;
        Matrix3 fpgun_orient;
        Vector3 fpgun_pos;
        float goal_sway_xrot;
        float cur_sway_xrot;
        float goal_sway_yrot;
        float cur_sway_yrot;
        int pivot_tag_handle;
        Timestamp update_silencer_state;
        bool show_silencer;
        int grenade_mode;
        bool remote_charge_in_hand;
        Vector3 breaking_riot_shield_pos;
        Matrix3 breaking_riot_shield_orient;
        int shoulder_cannon_reload_tag;
        Timestamp unholster_done_timestamp;
        int fpgun_weapon_type;
    };
    static_assert(sizeof(PlayerFpgunData) == 0x104);

    struct PlayerIrData
    {
        bool cleared;
        int ir_bitmap_handle;
        int counter;
        int ir_weapon_type;
    };
    static_assert(sizeof(PlayerIrData) == 0x10);

    struct Player_1094
    {
        Vector3 field_0;
        Matrix3 field_C;
    };

    enum Team
    {
        TEAM_RED = 0,
        TEAM_BLUE = 1,
    };

    typedef int PlayerFlags;

    struct PlayerViewport
    {
        int clip_x;
        int clip_y;
        int clip_w;
        int clip_h;
        float fov_h;
    };

    struct Player
    {
        struct Player *next;
        struct Player *prev;
        String name;
        PlayerFlags flags;
        int entity_handle;
        int entity_type;
        Vector3 field_1C;
        int field_28;
        PlayerLevelStats *stats;
        ubyte team;
        bool collides_with_world;
        VMesh *weapon_mesh_handle;
        VMesh *last_weapon_mesh_handle;
        Timestamp next_idle;
        int muzzle_tag_index[2];
        int ammo_digit1_tag_index;
        int ammo_digit2_tag_index;
        bool key_items[32];
        int field_70[16];
        bool just_landed;
        bool is_crouched;
        int view_from_handle;
        Timestamp remote_charge_switch_timestamp;
        Timestamp use_key_timestamp;
        Timestamp spawn_protection_timestamp;
        Camera *cam;
        PlayerViewport viewport;
        int viewport_mode;
        int flashlight_light;
        PlayerSettings settings;
        int field_F6C;
        int field_F70;
        int field_F74;
        int field_F78;
        int field_F7C;
        PlayerFpgunData fpgun_data;
        PlayerIrData ir_data;
        Player_1094 field_1094;
        int field_10C4[3];
        Color screen_flash_color;
        int screen_flash_alpha;
        float fpgun_total_transition_time;
        float fpgun_elapsed_transition_time;
        int fpgun_current_state_anim;
        int fpgun_next_state_anim;
        void* shield_decals[26];
        int exposure_damage_sound_handle;
        int weapon_prefs[32];
        float death_fade_alpha;
        float death_fade_time_sec;
        void (*death_fade_callback)(Player*);
        float damage_indicator_alpha[4];
        int field_11F0;
        float field_11F4;
        int field_11F8;
        int field_11FC;
        PlayerNetData *nw_data;
    };
    static_assert(sizeof(Player) == 0x1204);

    static auto& player_list = addr_as_ref<Player*>(0x007C75CC);
    static auto& local_player = addr_as_ref<Player*>(0x007C75D4);

    static auto& multi_kill_local_player = addr_as_ref<void()>(0x004757A0);
    static auto& control_config_check_pressed =
        addr_as_ref<bool(ControlConfig *cc, ControlAction action, bool *was_pressed)>(0x0043D4F0);
    static auto& player_from_entity_handle = addr_as_ref<Player*(int entity_handle)>(0x004A3740);
    static auto& player_is_dead = addr_as_ref<bool(Player *player)>(0x004A4920);
    static auto& player_is_dying = addr_as_ref<bool(Player *player)>(0x004A4940);
    static auto& player_add_score = addr_as_ref<void(Player *player, int delta)>(0x004A7460);
    static auto& player_get_ai = addr_as_ref<AiInfo*(Player *player)>(0x004A3260);
    static auto& player_get_weapon_total_ammo = addr_as_ref<int(Player *player, int weapon_type)>(0x004A3280);
    static auto& player_fpgun_render = addr_as_ref<void(Player*)>(0x004A2B30);
    static auto& player_update = addr_as_ref<void(Player*)>(0x004A2700);
    static auto& player_fpgun_setup_mesh = addr_as_ref<void(Player*, int weapon_type)>(0x004AA230);
    static auto& player_fpgun_process = addr_as_ref<void(Player*)>(0x004AA6D0);
    static auto& player_fpgun_render_ir = addr_as_ref<void(Player* player)>(0x004AEEF0);
    static auto& player_fpgun_set_state = addr_as_ref<void(Player* player, int state)>(0x004AA560);
    static auto& player_fpgun_has_state = addr_as_ref<bool(Player* player, int state)>(0x004A9520);
    static auto& player_fpgun_clear_all_action_anim_sounds = addr_as_ref<void(rf::Player*)>(0x004A9490);
}
