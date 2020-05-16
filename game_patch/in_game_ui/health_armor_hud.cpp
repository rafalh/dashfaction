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

        int health_armor_font = g_big_health_armor_hud ? rf::hud_big_font : rf::hud_small_font;
        char buf[256];

        if (rf::EntityIsAttachedToVehicle(entity)) {
            rf::HudDrawDamageIndicators(player);
            auto vehicle = rf::EntityGetByHandle(entity->parent_handle);
            if (rf::EntityIsJeepDriver(entity) || rf::EntityIsJeepShooter(entity)) {
                rf::GrSetColor(255, 255, 255, 120);
                rf::GrBitmap(rf::hud_health_jeep_bmh, rf::hud_points[42].x, rf::hud_points[42].y, rf::gr_bitmap_clamp_state);
                rf::GrBitmap(rf::hud_health_veh_frame_bmh, rf::hud_points[43].x, rf::hud_points[43].y, rf::gr_bitmap_clamp_state);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                std::sprintf(buf, "%d", veh_life);
                rf::GrSetColorPtr(&rf::hud_full_color);
                rf::GrString(rf::hud_points[44].x, rf::hud_points[44].y, buf, health_armor_font);
            }
            else if (rf::EntityIsDriller(vehicle)) {
                rf::GrSetColor(255, 255, 255, 120);
                rf::GrBitmap(rf::hud_health_driller_bmh, rf::hud_points[45].x, rf::hud_points[45].y, rf::gr_bitmap_clamp_state);
                rf::GrBitmap(rf::hud_health_veh_frame_bmh, rf::hud_points[46].x, rf::hud_points[46].y, rf::gr_bitmap_clamp_state);
                auto veh_life = std::max(static_cast<int>(vehicle->life), 1);
                std::sprintf(buf, "%d", veh_life);
                rf::GrSetColorPtr(&rf::hud_full_color);
                rf::GrString(rf::hud_points[47].x, rf::hud_points[47].y, buf, health_armor_font);
            }
        }
        else {
            float scale = 1.875f;
            rf::GrSetColor(255, 255, 255, 120);
            int health_tex_idx = static_cast<int>(entity->life * 0.1f);
            health_tex_idx = std::clamp(health_tex_idx, 0, 10);
            int health_bmh = rf::hud_health_bitmaps[health_tex_idx];
            HudScaledBitmap(health_bmh,
                rf::hud_points[rf::hud_health].x * scale,
                rf::hud_points[rf::hud_health].y * scale,
                scale,
                rf::gr_bitmap_clamp_state);
            int enviro_tex_idx = static_cast<int>(entity->armor / entity->cls->envirosuit * 10.0f);
            enviro_tex_idx = std::clamp(enviro_tex_idx, 0, 10);
            int enviro_bmh = rf::hud_enviro_bitmaps[enviro_tex_idx];
            HudScaledBitmap(enviro_bmh,
                rf::hud_points[rf::hud_envirosuit].x * scale,
                rf::hud_points[rf::hud_envirosuit].y * scale,
                scale,
                rf::gr_bitmap_clamp_state);
            rf::GrSetColorPtr(&rf::hud_full_color);
            int health = std::max(entity->life, 1.0f);
            std::sprintf(buf, "%d", health);
            int text_w, text_h;
            rf::GrGetTextWidth(&text_w, &text_h, buf, -1, health_armor_font);
            rf::GrString(
                rf::hud_points[rf::hud_health_value_ul_corner].x * scale +
                    (rf::hud_points[rf::hud_health_value_width_and_height].x * scale - text_w) / 2,
                rf::hud_points[rf::hud_health_value_ul_corner].y * scale,
                buf,
                health_armor_font);
            rf::GrSetColorPtr(&rf::hud_mid_color);
            std::sprintf(buf, "%d", static_cast<int>(entity->armor));
            rf::GrGetTextWidth(&text_w, &text_h, buf, -1, health_armor_font);
            rf::GrString(
                rf::hud_points[rf::hud_envirosuit_value_ul_corner].x * scale +
                    (rf::hud_points[rf::hud_envirosuit_value_width_and_height].x * scale - text_w) / 2,
                rf::hud_points[rf::hud_envirosuit_value_ul_corner].y * scale,
                buf,
                health_armor_font);

            rf::HudDrawDamageIndicators(player);

            if (rf::EntityIsHoldingBody(entity)) {
                rf::GrSetColor(255, 255, 255, 255);
                rf::GrRenderState state{
                    rf::TEXTURE_SOURCE_WRAP,
                    rf::COLOR_OP_MUL,
                    rf::ALPHA_OP_MUL,
                    rf::ALPHA_BLEND_ADDITIVE,
                    rf::ZBUFFER_TYPE_NONE,
                    rf::FOG_TYPE_FORCE_OFF,
                };
                rf::GrBitmap(rf::hud_body_indicator_bmh, rf::hud_points[40].x, rf::hud_points[40].y * scale, state);
                rf::GrSetColorPtr(&rf::hud_body_color);
                rf::GrString(rf::hud_points[41].x, rf::hud_points[41].y * scale, rf::strings::array[5]);
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
