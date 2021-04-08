#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include "../rf/gr/gr.h"
#include "../rf/hud.h"
#include "../rf/entity.h"
#include "../rf/weapon.h"
#include "../rf/gr/gr_font.h"
#include "../rf/player/player.h"
#include "../rf/multi.h"
#include "../main/main.h"
#include "../os/console.h"
#include "hud_internal.h"

float g_hud_ammo_scale = 1.0f;

CallHook<void(int, int, int, rf::gr::Mode)> hud_render_ammo_gr_bitmap_hook{
    {
        // hud_render_ammo_clip
        0x0043A5E9u,
        0x0043A637u,
        0x0043A680u,
        // hud_render_ammo_power
        0x0043A988u,
        0x0043A9DDu,
        0x0043AA24u,
        // hud_render_ammo_no_clip
        0x0043AE80u,
        0x0043AEC3u,
        0x0043AF0Au,
    },
    [](int bm_handle, int x, int y, rf::gr::Mode mode) {
        hud_scaled_bitmap(bm_handle, x, y, g_hud_ammo_scale, mode);
    },
};

CallHook<void(int, int, int, rf::gr::Mode)> render_reticle_gr_bitmap_hook{
    {
        0x0043A499,
        0x0043A4FE,
    },
    [](int bm_handle, int x, int y, rf::gr::Mode mode) {
        float base_scale = g_game_config.big_hud ? 2.0f : 1.0f;
        float scale = base_scale * g_game_config.reticle_scale;

        x = static_cast<int>((x - rf::gr::clip_width() / 2) * scale) + rf::gr::clip_width() / 2;
        y = static_cast<int>((y - rf::gr::clip_height() / 2) * scale) + rf::gr::clip_height() / 2;

        hud_scaled_bitmap(bm_handle, x, y, scale, mode);
    },
};

FunHook<void(rf::Entity*, int, int, bool)> hud_render_ammo_hook{
    0x0043A510,
    [](rf::Entity *entity, int weapon_type, int offset_y, bool is_inactive) {
        offset_y = static_cast<int>(offset_y * g_hud_ammo_scale);
        hud_render_ammo_hook.call_target(entity, weapon_type, offset_y, is_inactive);
    },
};

void hud_weapons_set_big(bool is_big)
{
    rf::HudItem ammo_hud_items[] = {
        rf::hud_ammo_bar,
        rf::hud_ammo_signal,
        rf::hud_ammo_icon,
        rf::hud_ammo_in_clip_text_ul_region_coord,
        rf::hud_ammo_in_clip_text_width_and_height,
        rf::hud_ammo_in_inv_text_ul_region_coord,
        rf::hud_ammo_in_inv_text_width_and_height,
        rf::hud_ammo_bar_position_no_clip,
        rf::hud_ammo_signal_position_no_clip,
        rf::hud_ammo_icon_position_no_clip,
        rf::hud_ammo_in_inv_ul_region_coord_no_clip,
        rf::hud_ammo_in_inv_text_width_and_height_no_clip,
        rf::hud_ammo_in_clip_ul_coord,
        rf::hud_ammo_in_clip_width_and_height,
    };
    g_hud_ammo_scale = is_big ? 1.875f : 1.0f;
    for (auto item_num : ammo_hud_items) {
        rf::hud_coords[item_num] = hud_scale_coords(rf::hud_coords[item_num], g_hud_ammo_scale);
    }
    rf::hud_ammo_font = rf::gr::load_font(is_big ? "biggerfont.vf" : "bigfont.vf");
}

ConsoleCommand2 reticle_scale_cmd{
    "reticle_scale",
    [](std::optional<float> scale_opt) {
        if (scale_opt) {
            g_game_config.reticle_scale = scale_opt.value();
            g_game_config.save();
        }
        rf::console::printf("Reticle scale %.4f", g_game_config.reticle_scale.value());
    },
    "Sets/gets reticle scale",
};

bool hud_weapons_is_double_ammo()
{
    if (rf::is_multi) {
        return false;
    }
    auto entity = rf::entity_from_handle(rf::local_player->entity_handle);
    if (!entity) {
        return false;
    }
    auto weapon_type = entity->ai.current_primary_weapon;
    return weapon_type == rf::machine_pistol_weapon_type || weapon_type == rf::machine_pistol_special_weapon_type;
}

void hud_weapons_apply_patches()
{
    // Big HUD support
    hud_render_ammo_gr_bitmap_hook.install();
    hud_render_ammo_hook.install();
    render_reticle_gr_bitmap_hook.install();

    // Commands
    reticle_scale_cmd.register_cmd();
}
