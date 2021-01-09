#include "hud_internal.h"
#include "../rf/hud.h"
#include "../rf/player.h"
#include "../rf/entity.h"
#include "../rf/gameseq.h"
#include "../rf/gr.h"
#include "../rf/gr_font.h"
#include "../rf/localize.h"
#include <patch_common/FunHook.h>
#include <patch_common/MemUtils.h>

bool g_big_health_armor_hud = false;

FunHook<void(rf::Player*)> hud_status_render_hook{
    0x00439D80,
    [](rf::Player *player) {
        if (!g_big_health_armor_hud) {
            hud_status_render_hook.call_target(player);
            return;
        }

        auto entity = rf::entity_from_handle(player->entity_handle);
        if (!entity) {
            return;
        }

        if (!rf::gameseq_in_gameplay()) {
            return;
        }

        int font_id = rf::hud_status_font;
        // Note: 2x scale does not look good because bigfont is not exactly 2x version of smallfont
        float scale = 1.875f;

        if (rf::entity_is_attached_to_vehicle(entity)) {
            rf::hud_draw_damage_indicators(player);
            auto vehicle = rf::entity_from_handle(entity->host_handle);
            if (rf::entity_is_jeep_driver(entity) || rf::entity_is_jeep_shooter(entity)) {
                rf::gr_set_color(255, 255, 255, 120);
                auto [jeep_x, jeep_y] = hud_scale_coords(rf::hud_coords[rf::hud_jeep], scale);
                hud_scaled_bitmap(rf::hud_health_jeep_bmh, jeep_x, jeep_y, scale);
                auto [jeep_frame_x, jeep_frame_y] = hud_scale_coords(rf::hud_coords[rf::hud_jeep_frame], scale);
                hud_scaled_bitmap(rf::hud_health_veh_frame_bmh, jeep_frame_x, jeep_frame_y, scale);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                auto veh_life_str = std::to_string(veh_life);
                rf::gr_set_color(rf::hud_full_color);
                auto [jeep_value_x, jeep_value_y] = hud_scale_coords(rf::hud_coords[rf::hud_jeep_value], scale);
                rf::gr_string(jeep_value_x, jeep_value_y, veh_life_str.c_str(), font_id);
            }
            else if (rf::entity_is_driller(vehicle)) {
                rf::gr_set_color(255, 255, 255, 120);
                auto [driller_x, driller_y] = hud_scale_coords(rf::hud_coords[rf::hud_driller], scale);
                hud_scaled_bitmap(rf::hud_health_driller_bmh, driller_x, driller_y, scale);
                auto [driller_frame_x, driller_frame_y] = hud_scale_coords(rf::hud_coords[rf::hud_driller_frame], scale);
                hud_scaled_bitmap(rf::hud_health_veh_frame_bmh, driller_frame_x, driller_frame_y, scale);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                auto veh_life_str = std::to_string(veh_life);
                rf::gr_set_color(rf::hud_full_color);
                auto [driller_value_x, driller_value_y] = hud_scale_coords(rf::hud_coords[rf::hud_driller_value], scale);
                rf::gr_string(driller_value_x, driller_value_y, veh_life_str.c_str(), font_id);
            }
        }
        else {
            rf::gr_set_color(255, 255, 255, 120);
            int health_tex_idx = static_cast<int>(entity->life * 0.1f);
            health_tex_idx = std::clamp(health_tex_idx, 0, 10);
            int health_bmh = rf::hud_health_bitmaps[health_tex_idx];
            auto [health_x, health_y] = hud_scale_coords(rf::hud_coords[rf::hud_health], scale);
            hud_scaled_bitmap(health_bmh, health_x, health_y, scale);
            int enviro_tex_idx = static_cast<int>(entity->armor / entity->info->envirosuit * 10.0f);
            enviro_tex_idx = std::clamp(enviro_tex_idx, 0, 10);
            int enviro_bmh = rf::hud_enviro_bitmaps[enviro_tex_idx];
            auto [envirosuit_x, envirosuit_y] = hud_scale_coords(rf::hud_coords[rf::hud_envirosuit], scale);
            hud_scaled_bitmap(enviro_bmh, envirosuit_x, envirosuit_y, scale);
            rf::gr_set_color(rf::hud_full_color);
            int health = static_cast<int>(std::max(entity->life, 1.0f));
            auto health_str = std::to_string(health);
            int text_w, text_h;
            rf::gr_get_string_size(&text_w, &text_h, health_str.c_str(), -1, font_id);
            auto [health_value_x, health_value_y] = hud_scale_coords(rf::hud_coords[rf::hud_health_value_ul_corner], scale);
            auto health_value_w = hud_scale_coords(rf::hud_coords[rf::hud_health_value_width_and_height], scale).x;
            rf::gr_string(health_value_x + (health_value_w - text_w) / 2, health_value_y, health_str.c_str(), font_id);
            rf::gr_set_color(rf::hud_mid_color);
            auto armor_str = std::to_string(static_cast<int>(entity->armor));
            rf::gr_get_string_size(&text_w, &text_h, armor_str.c_str(), -1, font_id);
            auto [envirosuit_value_x, envirosuit_value_y] = hud_scale_coords(rf::hud_coords[rf::hud_envirosuit_value_ul_corner], scale);
            auto envirosuit_value_w = hud_scale_coords(rf::hud_coords[rf::hud_envirosuit_value_width_and_height], scale).x;
            rf::gr_string(envirosuit_value_x + (envirosuit_value_w - text_w) / 2, envirosuit_value_y, armor_str.c_str(), font_id);

            rf::hud_draw_damage_indicators(player);

            if (rf::entity_is_holding_body(entity)) {
                rf::gr_set_color(255, 255, 255, 255);
                static const rf::GrMode state{
                    rf::TEXTURE_SOURCE_WRAP,
                    rf::COLOR_SOURCE_VERTEX_TIMES_TEXTURE,
                    rf::ALPHA_SOURCE_VERTEX_TIMES_TEXTURE,
                    rf::ALPHA_BLEND_ADDITIVE,
                    rf::ZBUFFER_TYPE_NONE,
                    rf::FOG_NOT_ALLOWED,
                };
                auto [corpse_icon_x, corpse_icon_y] = hud_scale_coords(rf::hud_coords[rf::hud_corpse_icon], scale);
                hud_scaled_bitmap(rf::hud_body_indicator_bmh, corpse_icon_x, corpse_icon_y, scale, state);
                rf::gr_set_color(rf::hud_body_color);
                auto [corpse_text_x, corpse_text_y] = hud_scale_coords(rf::hud_coords[rf::hud_corpse_text], scale);
                rf::gr_string(corpse_text_x, corpse_text_y, rf::strings::array[5], font_id);
            }
        }
    },
};

void hud_status_apply_patches()
{
    hud_status_render_hook.install();
}

void hud_status_set_big(bool is_big)
{
    g_big_health_armor_hud = is_big;
    rf::hud_status_font = rf::gr_load_font(is_big ? "bigfont.vf" : "smallfont.vf");
    static bool big_bitmaps_preloaded = false;
    if (is_big && !big_bitmaps_preloaded) {
        for (int i = 0; i <= 10; ++i) {
            hud_preload_scaled_bitmap(rf::hud_health_bitmaps[i]);
        }
        for (int i = 0; i <= 10; ++i) {
            hud_preload_scaled_bitmap(rf::hud_enviro_bitmaps[i]);
        }
        big_bitmaps_preloaded = true;
    }
}
