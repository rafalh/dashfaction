#pragma once

#include <patch_common/MemUtils.h>
#include "../math/vector.h"
#include "../math/matrix.h"
#include "../os/timestamp.h"
#include "../os/string.h"
#include "../gr/gr.h"
#include "control_config.h"
#include "player_fpgun.h"

namespace rf
{
    // Forward declarations
    struct VMesh;
    struct PlayerNetData;
    struct AiInfo;
    struct Camera;

    /* Settings */

    struct PlayerSettings
    {
        ControlConfig controls;
        bool field_e50;
        bool field_e51;
        bool render_fpgun;
        bool weapons_sway;
        bool toggle_crouch;
        bool field_e55;
        bool field_e56;
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
        bool field_e61;
        bool field_e62;
        bool field_e63;
        bool bilinear_filtering;
        int detail_level;
        int textures_resolution_level;
        int character_detail_level;
        float field_e74;
        int multi_character;
        char name[32];
    };
    static_assert(sizeof(PlayerSettings) == 0xE9C);

    /* Player */

    struct PlayerLevelStats
    {
        int16_t prev_score;
        int16_t score;
        int16_t caps;
        bool took_part_in_flag_capture;
    };
    static_assert(sizeof(PlayerLevelStats) == 0x8);

    struct PlayerIrData
    {
        bool cleared;
        int ir_bitmap_handle;
        int counter;
        int ir_weapon_type;
    };
    static_assert(sizeof(PlayerIrData) == 0x10);

    struct PlayerCockpitData
    {
        Vector3 camera_pos;
        Matrix3 camera_orient;
        int camera_index;
        float chaingun_angle;
        float chaingun_rot_vel;
    };
    static_assert(sizeof(PlayerCockpitData) == 0x3C);

    enum Team
    {
        TEAM_RED = 0,
        TEAM_BLUE = 1,
    };

    struct PlayerViewport
    {
        int clip_x;
        int clip_y;
        int clip_w;
        int clip_h;
        float fov_h;
    };

    enum PlayerFlags
    {
        PF_KILL_AFTER_BLACKOUT = 0x200,
        PF_END_LEVEL_AFTER_BLACKOUT = 0x1000,
    };

    struct Player
    {
        struct Player *next;
        struct Player *prev;
        String name;
        unsigned flags;
        int entity_handle;
        int entity_type;
        Vector3 spew_pos;
        ubyte spew_vector_index;
        PlayerLevelStats *stats;
        ubyte team;
        bool collides_with_world;
        VMesh *weapon_mesh_handle;
        VMesh *last_weapon_mesh_handle;
        Timestamp next_idle;
        int muzzle_tag_index[2];
        int ammo_digit1_tag_index;
        int ammo_digit2_tag_index;
        bool key_items[96];
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
        PlayerFpgunData fpgun_data;
        PlayerIrData ir_data;
        PlayerCockpitData cockpit_data;
        Color screen_flash_color;
        int screen_flash_alpha;
        float fpgun_total_transition_time;
        float fpgun_elapsed_transition_time;
        int fpgun_current_state_anim;
        int fpgun_next_state_anim;
        void* shield_decals[25];
        bool field_114C;
        int exposure_damage_sound_handle;
        int weapon_prefs[32];
        float death_fade_alpha;
        float death_fade_time_sec;
        void (*death_fade_callback)(Player*);
        float damage_indicator_alpha[4];
        int field_11F0;
        float field_11F4;
        int field_11F8;
        ubyte last_damage_dir;
        PlayerNetData *net_data;
    };
    static_assert(sizeof(Player) == 0x1204);

    static auto& player_list = addr_as_ref<Player*>(0x007C75CC);
    static auto& local_player = addr_as_ref<Player*>(0x007C75D4);

    static auto& player_from_entity_handle = addr_as_ref<Player*(int entity_handle)>(0x004A3740);
    static auto& player_is_dead = addr_as_ref<bool(Player *player)>(0x004A4920);
    static auto& player_is_dying = addr_as_ref<bool(Player *player)>(0x004A4940);
    static auto& player_add_score = addr_as_ref<void(Player *player, int delta)>(0x004A7460);
    static auto& player_get_ai = addr_as_ref<AiInfo*(Player *player)>(0x004A3260);
    static auto& player_get_weapon_total_ammo = addr_as_ref<int(Player *player, int weapon_type)>(0x004A3280);
    static auto& player_render = addr_as_ref<void(Player*)>(0x004A2B30);
    static auto& player_render_held_corpse = addr_as_ref<void(Player* player)>(0x004A2B90);
    static auto& player_do_frame = addr_as_ref<void(Player*)>(0x004A2700);
    static auto& player_make_weapon_current_selection = addr_as_ref<void(Player *player, int weapon_type)>(0x004A4980);
    static auto& player_start_death_fade = addr_as_ref<void(Player *pp, float time_sec, void (*callback)(Player *))>(0x004A73E0);
    static auto& get_player_entity_parent_vmesh = addr_as_ref<VMesh*(Player*)>(0x004A7830);
}
