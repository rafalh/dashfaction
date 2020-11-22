#include "hud_internal.h"
#include "../rf/hud.h"
#include "../rf/player.h"
#include "../rf/entity.h"
#include "../rf/gameseq.h"
#include "../rf/graphics.h"
#include "../rf/localize.h"
#include <patch_common/FunHook.h>
#include <patch_common/MemUtils.h>

bool g_big_health_armor_hud = false;

FunHook<void(rf::Player*)> HudRenderHealthAndEnviro_hook{
    0x00439D80,
    [](rf::Player *player) {
        if (!g_big_health_armor_hud) {
            HudRenderHealthAndEnviro_hook.CallTarget(player);
            return;
        }

        auto entity = rf::EntityFromHandle(player->entity_handle);
        if (!entity) {
            return;
        }

        if (!rf::GameSeqInGameplay()) {
            return;
        }

        int font_id = rf::hud_health_enviro_font;
        // Note: 2x scale does not look good because bigfont is not exactly 2x version of smallfont
        float scale = 1.875f;

        if (rf::EntityIsAttachedToVehicle(entity)) {
            rf::HudDrawDamageIndicators(player);
            auto vehicle = rf::EntityFromHandle(entity->host_handle);
            if (rf::EntityIsJeepDriver(entity) || rf::EntityIsJeepShooter(entity)) {
                rf::GrSetColorRgba(255, 255, 255, 120);
                auto [jeep_x, jeep_y] = HudScaleCoords(rf::hud_points[rf::hud_jeep], scale);
                HudScaledBitmap(rf::hud_health_jeep_bmh, jeep_x, jeep_y, scale);
                auto [jeep_frame_x, jeep_frame_y] = HudScaleCoords(rf::hud_points[rf::hud_jeep_frame], scale);
                HudScaledBitmap(rf::hud_health_veh_frame_bmh, jeep_frame_x, jeep_frame_y, scale);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                auto veh_life_str = std::to_string(veh_life);
                rf::GrSetColor(rf::hud_full_color);
                auto [jeep_value_x, jeep_value_y] = HudScaleCoords(rf::hud_points[rf::hud_jeep_value], scale);
                rf::GrString(jeep_value_x, jeep_value_y, veh_life_str.c_str(), font_id);
            }
            else if (rf::EntityIsDriller(vehicle)) {
                rf::GrSetColorRgba(255, 255, 255, 120);
                auto [driller_x, driller_y] = HudScaleCoords(rf::hud_points[rf::hud_driller], scale);
                HudScaledBitmap(rf::hud_health_driller_bmh, driller_x, driller_y, scale);
                auto [driller_frame_x, driller_frame_y] = HudScaleCoords(rf::hud_points[rf::hud_driller_frame], scale);
                HudScaledBitmap(rf::hud_health_veh_frame_bmh, driller_frame_x, driller_frame_y, scale);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                auto veh_life_str = std::to_string(veh_life);
                rf::GrSetColor(rf::hud_full_color);
                auto [driller_value_x, driller_value_y] = HudScaleCoords(rf::hud_points[rf::hud_driller_value], scale);
                rf::GrString(driller_value_x, driller_value_y, veh_life_str.c_str(), font_id);
            }
        }
        else {
            rf::GrSetColorRgba(255, 255, 255, 120);
            int health_tex_idx = static_cast<int>(entity->life * 0.1f);
            health_tex_idx = std::clamp(health_tex_idx, 0, 10);
            int health_bmh = rf::hud_health_bitmaps[health_tex_idx];
            auto [health_x, health_y] = HudScaleCoords(rf::hud_points[rf::hud_health], scale);
            HudScaledBitmap(health_bmh, health_x, health_y, scale);
            int enviro_tex_idx = static_cast<int>(entity->armor / entity->info->envirosuit * 10.0f);
            enviro_tex_idx = std::clamp(enviro_tex_idx, 0, 10);
            int enviro_bmh = rf::hud_enviro_bitmaps[enviro_tex_idx];
            auto [envirosuit_x, envirosuit_y] = HudScaleCoords(rf::hud_points[rf::hud_envirosuit], scale);
            HudScaledBitmap(enviro_bmh, envirosuit_x, envirosuit_y, scale);
            rf::GrSetColor(rf::hud_full_color);
            int health = static_cast<int>(std::max(entity->life, 1.0f));
            auto health_str = std::to_string(health);
            int text_w, text_h;
            rf::GrGetStringSize(&text_w, &text_h, health_str.c_str(), -1, font_id);
            auto [health_value_x, health_value_y] = HudScaleCoords(rf::hud_points[rf::hud_health_value_ul_corner], scale);
            auto health_value_w = HudScaleCoords(rf::hud_points[rf::hud_health_value_width_and_height], scale).x;
            rf::GrString(health_value_x + (health_value_w - text_w) / 2, health_value_y, health_str.c_str(), font_id);
            rf::GrSetColor(rf::hud_mid_color);
            auto armor_str = std::to_string(static_cast<int>(entity->armor));
            rf::GrGetStringSize(&text_w, &text_h, armor_str.c_str(), -1, font_id);
            auto [envirosuit_value_x, envirosuit_value_y] = HudScaleCoords(rf::hud_points[rf::hud_envirosuit_value_ul_corner], scale);
            auto envirosuit_value_w = HudScaleCoords(rf::hud_points[rf::hud_envirosuit_value_width_and_height], scale).x;
            rf::GrString(envirosuit_value_x + (envirosuit_value_w - text_w) / 2, envirosuit_value_y, armor_str.c_str(), font_id);

            rf::HudDrawDamageIndicators(player);

            if (rf::EntityIsHoldingBody(entity)) {
                rf::GrSetColorRgba(255, 255, 255, 255);
                static const rf::GrMode state{
                    rf::TEXTURE_SOURCE_WRAP,
                    rf::COLOR_SOURCE_VERTEX_TIMES_TEXTURE,
                    rf::ALPHA_SOURCE_VERTEX_TIMES_TEXTURE,
                    rf::ALPHA_BLEND_ADDITIVE,
                    rf::ZBUFFER_TYPE_NONE,
                    rf::FOG_NOT_ALLOWED,
                };
                auto [corpse_icon_x, corpse_icon_y] = HudScaleCoords(rf::hud_points[rf::hud_corpse_icon], scale);
                HudScaledBitmap(rf::hud_body_indicator_bmh, corpse_icon_x, corpse_icon_y, scale, state);
                rf::GrSetColor(rf::hud_body_color);
                auto [corpse_text_x, corpse_text_y] = HudScaleCoords(rf::hud_points[rf::hud_corpse_text], scale);
                rf::GrString(corpse_text_x, corpse_text_y, rf::strings::array[5], font_id);
            }
        }
    },
};

void InstallHealthArmorHudPatches()
{
    HudRenderHealthAndEnviro_hook.Install();
}

void SetBigHealthArmorHud(bool is_big)
{
    g_big_health_armor_hud = is_big;
    rf::hud_health_enviro_font = rf::GrLoadFont(is_big ? "bigfont.vf" : "smallfont.vf");
    static bool big_bitmaps_preloaded = false;
    if (is_big && !big_bitmaps_preloaded) {
        for (int i = 0; i <= 10; ++i) {
            HudPreloadScaledBitmap(rf::hud_health_bitmaps[i]);
        }
        for (int i = 0; i <= 10; ++i) {
            HudPreloadScaledBitmap(rf::hud_enviro_bitmaps[i]);
        }
        big_bitmaps_preloaded = true;
    }
}
