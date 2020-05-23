#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"
#include "graphics.h"

namespace rf
{
    struct AnimMesh;
    struct Player;

    struct HudPoint
    {
        int x;
        int y;
    };

    constexpr int num_hud_points = 48;
    static auto& hud_points_640 = AddrAsRef<HudPoint[num_hud_points]>(0x00637868);
    static auto& hud_points_800 = AddrAsRef<HudPoint[num_hud_points]>(0x006373D0);
    static auto& hud_points_1024 = AddrAsRef<HudPoint[num_hud_points]>(0x00637230);
    static auto& hud_points_1280 = AddrAsRef<HudPoint[num_hud_points]>(0x00637560);
    static auto& hud_points = AddrAsRef<HudPoint[num_hud_points]>(0x006376E8);

    static auto& hud_ammo_font = AddrAsRef<int>(0x00596F64);
    static auto& hud_health_enviro_font = AddrAsRef<int>(0x00596F68);
    static auto& hud_msg_font_num = AddrAsRef<int>(0x005A1350);
    static auto& hud_text_font_num = AddrAsRef<int>(0x005A1358);

    static auto& is_hud_hidden = AddrAsRef<bool>(0x006379F0);
    static auto& scoreboard_visible = AddrAsRef<bool>(0x006A1448);

    enum HudItem
    {
        hud_health                                    = 0,
        hud_health_value_ul_corner                    = 1,
        hud_health_value_width_and_height             = 2,
        hud_envirosuit                                = 3,
        hud_envirosuit_value_ul_corner                = 4,
        hud_envirosuit_value_width_and_height         = 5,
        hud_ammo_bar                                  = 6,
        hud_ammo_signal                               = 7, // behind_icon
        hud_ammo_icon                                 = 8,
        hud_ammo_in_clip_text_ul_region_coord         = 9,
        hud_ammo_in_clip_text_width_and_height        = 10,
        hud_ammo_in_inv_text_ul_region_coord          = 11,
        hud_ammo_in_inv_text_width_and_height         = 12,
        hud_reticle                                   = 13,
        hud_ammo_bar_position_no_clip                 = 14, // with no clip, i.e. when using rockets
        hud_ammo_signal_position_no_clip              = 15, // with no clip
        hud_ammo_icon_position_no_clip                = 16, // with no clip
        hud_ammo_in_inv_ul_region_coord_no_clip       = 17, // with no clip
        hud_ammo_in_inv_text_width_and_height_no_clip = 18, // with no clip
        hud_scope_center_circle_radius                = 19, // along x and y axes
        hud_front_damage_indicator_center             = 20,
        hud_left_damage_indicator_center              = 21,
        hud_back_damage_indicator_center              = 22,
        hud_right_damage_indicator_center             = 23,
        hud_ammo_in_clip_ul_coord                     = 24,
        hud_ammo_in_clip_width_and_height             = 25,
        hud_front_damage_indicator_center_splitscreen = 26, // relative to clip region
        hud_left_damage_indicator_center_splitscreen  = 27, // relative to clip region
        hud_back_damage_indicator_center_splitscreen  = 28, // relative to clip region
        hud_right_damage_indicator_center_splitscreen = 29, // relative to clip region
        hud_persona_message_box_background_ul         = 30,
        hud_persona_message_box_background_w_h        = 31,
        hud_persona_message_box_text_area_upper_left  = 32,
        hud_persona_message_box_text_area_width       = 33, // height ignored
        hud_persona_head_image                        = 34,
        hud_persona_head_image_name                   = 35,
        hud_persona_sub_offset                        = 36,
        hud_message_width_and_upper_y_value           = 37, // they render centered
        hud_message_log_text_area_width               = 38, // height ignored
        hud_countdown_timer                           = 39,
        hud_corpse_icon                               = 40,
        hud_corpse_text                               = 41,
        hud_jeep                                      = 42,
        hud_jeep_frame                                = 43,
        hud_jeep_value                                = 44,
        hud_driller                                   = 45,
        hud_driller_frame                             = 46,
        hud_driller_value                             = 47,
    };

    static auto& HudDrawDamageIndicators = AddrAsRef<void(Player*)>(0x00439CA0);

    static auto& hud_full_color = AddrAsRef<Color>(0x006373B8);
    static auto& hud_mid_color = AddrAsRef<Color>(0x006373BC);
    static auto& hud_low_color = AddrAsRef<Color>(0x006373C0);
    static auto& hud_body_color = AddrAsRef<Color>(0x00637228);
    static auto& hud_enviro_bitmaps = AddrAsRef<int[11]>(0x00639078);
    static auto& hud_health_bitmaps = AddrAsRef<int[11]>(0x006390A4);
    static auto& hud_health_jeep_bmh = AddrAsRef<int>(0x005974D0);
    static auto& hud_health_driller_bmh = AddrAsRef<int>(0x005974D4);
    static auto& hud_health_veh_frame_bmh = AddrAsRef<int>(0x005974D8);
    static auto& hud_body_indicator_bmh = AddrAsRef<int>(0x005974CC);

    struct ChatMsg
    {
        rf::String name;
        rf::String text;
        int color_id;
    };

    static auto& chat_fully_visible_timer = AddrAsRef<Timer>(0x006C9C7C);
    static auto& chat_fade_out_timer = AddrAsRef<Timer>(0x006C9C88);
    static auto& chat_messages = AddrAsRef<ChatMsg[8]>(0x006C9C98);

    struct HudPersonasTbl
    {
        String name;
        String image;
        String display_name;
        int image_bmh;
        int sound;
        int game_snd_id;
        char message[256];
        Timer fully_visible_timer;
    };

    static auto& hud_persona_alpha = AddrAsRef<float>(0x00597290);
    static auto& hud_persona_target_alpha = AddrAsRef<float>(0x0059728C);
    static auto& hud_persona_current_idx = AddrAsRef<int>(0x006384C4);
    static auto& hud_default_color = AddrAsRef<Color>(0x00637554);
    static auto& hud_msg_bg_color = AddrAsRef<Color>(0x006379EC);
    static auto& hud_msg_color = AddrAsRef<Color>(0x006373B4);
    static auto& hud_personas_tbl = AddrAsRef<HudPersonasTbl[10]>(0x006384D0);
    static auto& hud_persona_image_render_state = AddrAsRef<GrRenderState>(0x01775B18);

    struct WeaponCycle
    {
        int cls_id;
        AnimMesh *mesh;
        AnimMesh *fpgun_mesh;
        int type;
    };

    static auto& hud_close_weapon_cycle_timer = AddrAsRef<rf::Timer>(0x007C73B8);
    static auto& hud_weapon_cycle = AddrAsRef<rf::WeaponCycle[32]>(0x007C71B0);
    static auto& hud_render_weapon_cycle = AddrAsRef<bool>(0x007C7640);
    static auto& hud_weapon_display_off_foley_snd = AddrAsRef<int>(0x007C75D0);
    static auto& hud_weapon_cycle_current_idx = AddrAsRef<int>(0x007C71A8);
}
