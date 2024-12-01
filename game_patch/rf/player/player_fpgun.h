#pragma once

#include "../os/timestamp.h"
#include "../math/vector.h"
#include "../math/matrix.h"
#include "../gr/gr.h"

namespace rf
{
    struct Player;

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

    static auto& player_fpgun_set_state = addr_as_ref<void(Player* player, int weapon_type)>(0x004AA230);
    static auto& player_fpgun_process = addr_as_ref<void(Player* player)>(0x004AA6D0);
    static auto& player_fpgun_render_ir = addr_as_ref<void(Player* player)>(0x004AEEF0);
    static auto& player_fpgun_set_next_state_anim = addr_as_ref<void(Player* player, int anim_index)>(0x004AA560);
    static auto& player_fpgun_is_in_state_anim = addr_as_ref<bool(Player* player, int anim_index)>(0x004A9520);
    static auto& player_fpgun_clear_all_action_anim_sounds = addr_as_ref<void(Player* player)>(0x004A9490);
    static auto& player_fpgun_load_meshes = addr_as_ref<void()>(0x004AE530);
    static auto& player_fpgun_delete_meshes = addr_as_ref<void()>(0x004AEB40);
    static auto& player_fpgun_page_in = addr_as_ref<void(Player* player, int unused, int weapon_type)>(0x004AE350);
}
