#include "hud_internal.h"
#include "../rf/hud.h"
#include "../rf/player.h"
#include "../rf/entity.h"
#include "../rf/game_seq.h"
#include "../rf/graphics.h"
#include "../rf/misc.h"
#include <patch_common/FunHook.h>
#include <patch_common/MemUtils.h>

bool g_big_health_armor_hud = false;

int ScaleHudCoord(int val, int max, float scale)
{
    if (val < max / 3) {
        return static_cast<int>(val * scale);
    }
    else if (val < max * 2 / 3) {
        return max / 2 + static_cast<int>((val - max / 2) * scale);
    }
    else {
        return max + static_cast<int>((val - max) * scale);
    }
}

int ScaleHudX(int val, float scale)
{
    return ScaleHudCoord(val, rf::GrGetMaxWidth(), scale);
}

int ScaleHudY(int val, float scale)
{
    return ScaleHudCoord(val, rf::GrGetMaxWidth(), scale);
}

rf::HudPoint ScaleHudPoint(rf::HudPoint pt, float scale)
{
    return {
        ScaleHudX(pt.x, scale),
        ScaleHudY(pt.y, scale),
    };
}

FunHook<void(rf::Player*)> HudRenderHealthAndEnviro_hook{
    0x00439D80,
    [](rf::Player *player) {
        if (!g_big_health_armor_hud) {
            HudRenderHealthAndEnviro_hook.CallTarget(player);
            return;
        }

        auto entity = rf::EntityGetByHandle(player->entity_handle);
        if (!entity) {
            return;
        }

        if (!rf::GameSeqIsCurrentlyInGame()) {
            return;
        }

        int font_id = g_big_health_armor_hud ? rf::hud_big_font : rf::hud_small_font;
        float scale = 1.875f;

        if (rf::EntityIsAttachedToVehicle(entity)) {
            rf::HudDrawDamageIndicators(player);
            auto vehicle = rf::EntityGetByHandle(entity->parent_handle);
            if (rf::EntityIsJeepDriver(entity) || rf::EntityIsJeepShooter(entity)) {
                rf::GrSetColor(255, 255, 255, 120);
                auto [jeep_x, jeep_y] = ScaleHudPoint(rf::hud_points[rf::hud_jeep], scale);
                HudScaledBitmap(rf::hud_health_jeep_bmh, jeep_x, jeep_y, scale);
                auto [jeep_frame_x, jeep_frame_y] = ScaleHudPoint(rf::hud_points[rf::hud_jeep_frame], scale);
                HudScaledBitmap(rf::hud_health_veh_frame_bmh, jeep_frame_x, jeep_frame_y, scale);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                auto veh_life_str = std::to_string(veh_life);
                rf::GrSetColorPtr(&rf::hud_full_color);
                auto [jeep_value_x, jeep_value_y] = ScaleHudPoint(rf::hud_points[rf::hud_jeep_value], scale);
                rf::GrString(jeep_value_x, jeep_value_y, veh_life_str.c_str(), font_id);
            }
            else if (rf::EntityIsDriller(vehicle)) {
                rf::GrSetColor(255, 255, 255, 120);
                auto [driller_x, driller_y] = ScaleHudPoint(rf::hud_points[rf::hud_driller], scale);
                HudScaledBitmap(rf::hud_health_driller_bmh, driller_x, driller_y, scale);
                auto [driller_frame_x, driller_frame_y] = ScaleHudPoint(rf::hud_points[rf::hud_driller_frame], scale);
                HudScaledBitmap(rf::hud_health_veh_frame_bmh, driller_frame_x, driller_frame_y, scale);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                auto veh_life_str = std::to_string(veh_life);
                rf::GrSetColorPtr(&rf::hud_full_color);
                auto [driller_value_x, driller_value_y] = ScaleHudPoint(rf::hud_points[rf::hud_driller_value], scale);
                rf::GrString(driller_value_x, driller_value_y, veh_life_str.c_str(), font_id);
            }
        }
        else {
            rf::GrSetColor(255, 255, 255, 120);
            int health_tex_idx = static_cast<int>(entity->life * 0.1f);
            health_tex_idx = std::clamp(health_tex_idx, 0, 10);
            int health_bmh = rf::hud_health_bitmaps[health_tex_idx];
            auto [health_x, health_y] = ScaleHudPoint(rf::hud_points[rf::hud_health], scale);
            HudScaledBitmap(health_bmh, health_x, health_y, scale);
            int enviro_tex_idx = static_cast<int>(entity->armor / entity->cls->envirosuit * 10.0f);
            enviro_tex_idx = std::clamp(enviro_tex_idx, 0, 10);
            int enviro_bmh = rf::hud_enviro_bitmaps[enviro_tex_idx];
            auto [envirosuit_x, envirosuit_y] = ScaleHudPoint(rf::hud_points[rf::hud_envirosuit], scale);
            HudScaledBitmap(enviro_bmh, envirosuit_x, envirosuit_y, scale);
            rf::GrSetColorPtr(&rf::hud_full_color);
            int health = std::max(entity->life, 1.0f);
            auto health_str = std::to_string(health);
            int text_w, text_h;
            rf::GrGetTextWidth(&text_w, &text_h, health_str.c_str(), -1, font_id);
            auto [health_value_x, health_value_y] = ScaleHudPoint(rf::hud_points[rf::hud_health_value_ul_corner], scale);
            auto health_value_w = ScaleHudX(rf::hud_points[rf::hud_health_value_width_and_height].x, scale);
            rf::GrString(health_value_x + (health_value_w - text_w) / 2, health_value_y, health_str.c_str(), font_id);
            rf::GrSetColorPtr(&rf::hud_mid_color);
            auto armor_str = std::to_string(static_cast<int>(entity->armor));
            rf::GrGetTextWidth(&text_w, &text_h, armor_str.c_str(), -1, font_id);
            auto [envirosuit_value_x, envirosuit_value_y] = ScaleHudPoint(rf::hud_points[rf::hud_envirosuit_value_ul_corner], scale);
            auto envirosuit_value_w = ScaleHudX(rf::hud_points[rf::hud_envirosuit_value_width_and_height].x, scale);
            rf::GrString(envirosuit_value_x + (envirosuit_value_w - text_w) / 2, envirosuit_value_y, armor_str.c_str(), font_id);

            rf::HudDrawDamageIndicators(player);

            if (rf::EntityIsHoldingBody(entity)) {
                rf::GrSetColor(255, 255, 255, 255);
                static const rf::GrRenderState state{
                    rf::TEXTURE_SOURCE_WRAP,
                    rf::COLOR_OP_MUL,
                    rf::ALPHA_OP_MUL,
                    rf::ALPHA_BLEND_ADDITIVE,
                    rf::ZBUFFER_TYPE_NONE,
                    rf::FOG_TYPE_FORCE_OFF,
                };
                auto [corpse_icon_x, corpse_icon_y] = ScaleHudPoint(rf::hud_points[rf::hud_corpse_icon], scale);
                rf::GrBitmap(rf::hud_body_indicator_bmh, corpse_icon_x, corpse_icon_y, state);
                rf::GrSetColorPtr(&rf::hud_body_color);
                auto [corpse_text_x, corpse_text_y] = ScaleHudPoint(rf::hud_points[rf::hud_corpse_text], scale);
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
}
