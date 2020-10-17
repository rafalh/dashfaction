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

    static auto& CameraSetFirstPerson = AddrAsRef<bool(Camera *camera)>(0x0040DDF0);
    static auto& CameraSetFreelook = AddrAsRef<bool(Camera *camera)>(0x0040DCF0);
    static auto& CameraSetFixed = AddrAsRef<bool(Camera *camera)>(0x0040DF70);
    static auto& CameraGetPos = AddrAsRef<void(rf::Vector3* camera_pos, rf::Camera *camera)>(0x0040D760);
    static auto& CameraGetOrient = AddrAsRef<void(rf::Matrix3* camera_orient, rf::Camera *camera)>(0x0040D780);

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
        int mouse_look;
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
        char field_f34;
        char field_e51;
        char first_person_weapon_visible;
        char field_e53;
        char crouch_stay;
        char field_f39;
        char field_f3a;
        char hud_visible;
        char field_f3c;
        char field_e59;
        char field_e5a;
        char field_e5b;
        char autoswitch_weapons;
        char autoswitch_explosives;
        char shadows_enabled;
        char decals_enabled;
        char dynamic_lightining_enabled;
        char field_f45;
        char field_f46;
        char field_f47;
        char filtering_level;
        char field_f49;
        char field_f4a;
        char field_f4b;
        int detail_level;
        int textures_resolution_level;
        int character_detail_level;
        float field_f58;
        int mp_character;
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
        int next_weapon_cls_id;
        Timestamp weapon_switch_timestamp;
        Timestamp reload_timestamp;
        int field_C;
        Timestamp clip_drain_timestamp;
        bool in_scope_view;
        char field_15;
        char field_16;
        char field_17;
        float scope_zoom;
        char field_1C;
        char rocket_launcher_locked;
        char field_1E;
        char field_1F;
        int rocker_launcher_lock_obj_handle;
        Timestamp rocker_launcher_lock_timestamp;
        Timestamp rocket_scan_timestamp;
        Timestamp rocket_scan_timer_unk1;
        bool railgun_scanner_active;
        char scanner_view_rendering;
        Color scanner_mask_clr;
        char field_36;
        char field_37;
        int field_38;
        int field_3C;
        float unk_time_40;
        Vector3 field_44;
        Matrix3 field_50;
        Matrix3 fps_weapon_orient;
        Vector3 fps_weapon_pos;
        float field_A4;
        float field_A8;
        float field_AC;
        float field_B0;
        int pivot1_prop_id;
        Timestamp field_B8;
        int is_silenced_12mm_handgun;
        int field_C0;
        int remote_charge_visible;
        Vector3 field_C8;
        Matrix3 field_D4;
        int field_F8;
        Timestamp field_FC;
    };
    static_assert(sizeof(PlayerFpgunData) == 0x100);

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
        int entity_cls_id;
        Vector3 field_1C;
        int field_28;
        PlayerLevelStats *stats;
        char team;
        char collide;
        char field_32;
        char field_33;
        VMesh *fpgun_mesh;
        VMesh *last_fpgun_mesh;
        Timestamp fpgun_idle_timestamp;
        int fpgun_muzzle_props[2];
        int fpgun_ammo_digit1_prop;
        int fpgun_ammo_digit2_prop;
        char key_items[32];
        int field_70[16];
        char unk_landed;
        char is_crouched;
        char field_B2;
        char field_B3;
        int view_obj_handle;
        Timestamp weapon_switch_timer2;
        Timestamp use_key_timestamp;
        Timestamp spawn_protection_timestamp;
        Camera *cam;
        PlayerViewport viewport;
        int viewport_mode;
        int front_collision_light;
        PlayerSettings settings;
        int field_F6C;
        int field_F70;
        int field_F74;
        int field_F78;
        int field_F7C;
        PlayerFpgunData fpgun_data;
        int fpgun_weapon_id;
        char is_scanner_bm_empty;
        int scanner_bm_handle;
        int field_108C;
        int infrared_weapon_cls_id;
        Player_1094 field_1094;
        int field_10C4[3];
        Color screen_flash_color;
        int screen_flash_alpha;
        float field_10D8;
        float field_10DC;
        int field_10E0;
        int weapon_cls_state_id;
        void* riot_shield_hits[26];
        int gasp_outside_sound;
        int pref_weapons[32];
        float black_out_time;
        float black_out_time2;
        void (__cdecl *black_out_callback)(Player *);
        float damage_indicator_alpha[4];
        int field_11F0;
        float field_11F4;
        int field_11F8;
        int field_11FC;
        PlayerNetData *nw_data;
    };
    static_assert(sizeof(Player) == 0x1204);

    static auto& player_list = AddrAsRef<Player*>(0x007C75CC);
    static auto& local_player = AddrAsRef<Player*>(0x007C75D4);

    static auto& MultiKillLocalPlayer = AddrAsRef<void()>(0x004757A0);
    static auto& ControlConfigCheckPressed =
        AddrAsRef<bool(ControlConfig *cc, ControlAction action, bool *was_pressed)>(0x0043D4F0);
    static auto& GetPlayerFromEntityHandle = AddrAsRef<Player*(int entity_handle)>(0x004A3740);
    static auto& PlayerIsDead = AddrAsRef<bool(Player *player)>(0x004A4920);
    static auto& PlayerIsDying = AddrAsRef<bool(Player *player)>(0x004A4940);
    static auto& PlayerAddScore = AddrAsRef<void(Player *player, int delta)>(0x004A7460);
    static auto& PlayerGetAiInfo = AddrAsRef<AiInfo*(Player *player)>(0x004A3260);
    static auto& PlayerGetWeaponTotalAmmo = AddrAsRef<int(Player *player, int weapon_type)>(0x004A3280);
}
