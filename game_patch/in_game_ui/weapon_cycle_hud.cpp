#include "hud_internal.h"
#include "hud.h"
#include "../rf/hud.h"
#include "../rf/player.h"
#include "../rf/entity.h"
#include "../rf/graphics.h"
#include "../rf/weapon.h"
#include "../rf/misc.h"
#include "../rf/sound.h"
#include <patch_common/FunHook.h>

bool g_big_weapon_cycle_hud = false;

void RenderSelectWeaponGui()
{
    if (!rf::local_player) {
        return;
    }
    if (rf::hud_close_weapon_cycle_timer.IsFinished() || !rf::hud_close_weapon_cycle_timer.IsSet()) {
        rf::PlaySnd(rf::hud_weapon_display_off_foley_snd, 0, 0.0f, 1.0f);
        rf::hud_render_weapon_cycle = false;
        return;
    }

    int clip_w = rf::GrGetClipWidth();
    rf::GrSetColor(15, 242, 2, 255);
    auto font_num = HudGetDefaultFont();

    // selected weapon type name
    auto& selected_cycle_entry = rf::hud_weapon_cycle[rf::hud_weapon_cycle_current_idx];
    const char* weapon_type_name = nullptr;
    switch (selected_cycle_entry.type) {
    case 0:
        weapon_type_name = rf::strings::close_combat;
        break;
    case 1:
        weapon_type_name = rf::strings::semi_auto;
        break;
    case 2:
        weapon_type_name = rf::strings::heavy;
        break;
    case 3:
        weapon_type_name = rf::strings::explosive;
        break;
    default:
        break;
    }
    int weapon_type_y = g_big_weapon_cycle_hud ? 210 : 113;
    int center_x = clip_w - (g_big_weapon_cycle_hud ? 148 : 74);
    if (weapon_type_name) {
        rf::GrStringAligned(rf::GR_ALIGN_CENTER, center_x, weapon_type_y, weapon_type_name, font_num);
    }
    // weapon type squares
    int sq_bg_size = g_big_weapon_cycle_hud ? 30 : 20;
    int border = 1;
    int sq_spacing = 1;
    int sq_x_delta = sq_bg_size + 2 * border + sq_spacing; // 23
    int type_sq_y = weapon_type_y + (g_big_weapon_cycle_hud ? 24 : 14); // 127
    int type_sq_start_x = center_x - 2 * sq_x_delta; // w - 120
    int text_offset_x = sq_bg_size / 2;
    int text_offset_y = 5;

    // type squares - backgrounds
    rf::GrSetColor(0, 0, 0, 128);
    for (int i = 0; i < 4; ++i) {
        int sq_x = type_sq_start_x + i * sq_x_delta;
        rf::GrRect(sq_x + border, type_sq_y + border + 1, sq_bg_size, sq_bg_size);
    }
    // type squares - labels
    rf::GrSetColor(15, 242, 2, 255);
    for (int i = 0; i < 4; ++i) {
        char digit_str[] = {static_cast<char>('1' + i), '\0'};
        int sq_x = type_sq_start_x + i * sq_x_delta;
        int sq_text_x = sq_x + border + text_offset_x;
        rf::GrStringAligned(rf::GR_ALIGN_CENTER, sq_text_x, type_sq_y + text_offset_y, digit_str, font_num);
    }
    // borders for active type square
    int active_sq_x = type_sq_start_x + selected_cycle_entry.type * sq_x_delta;
    rf::GrSetColor(239, 213, 52, 255);
    int sq_with_border_size = sq_bg_size + 2 * border; // 22
    HudRectBorder(active_sq_x, type_sq_y, sq_with_border_size, sq_with_border_size, border);

    int num_ind_start_y = type_sq_y + sq_bg_size + 2 * border + sq_spacing + 1; // 150
    int weapon_icons_start_y = num_ind_start_y + 26; // 176
    float weapon_icon_scale = g_big_weapon_cycle_hud ? 1.5f : 1.0f;
    int weapon_icon_w = static_cast<int>(128 * weapon_icon_scale);
    int weapon_icon_h = static_cast<int>(34 * weapon_icon_scale);
    int weapon_icons_x = center_x - weapon_icon_w / 2; // clip_w - 139
    int weapon_icon_delta_y = weapon_icon_h + 1; // 35

    int num_drawn_weapons_per_type[4] = {0};
    auto ai_info = rf::PlayerGetAiInfo(rf::local_player);
    for (auto& pref_id : rf::local_player->pref_weapons) {
        rf::WeaponCycle* cycle_entry = nullptr;
        for (auto& entry : rf::hud_weapon_cycle) {
            if (entry.cls_id == pref_id) {
                cycle_entry = &entry;
            }
        }
        if (!cycle_entry || cycle_entry->cls_id == -1 ||  cycle_entry->type == -1) {
            continue;
        }
        if (!rf::AiPossessesWeapon(ai_info, cycle_entry->cls_id)) {
            continue;
        }
        int weapon_cls_id = cycle_entry->cls_id;
        int weapon_type = cycle_entry->type;
        if (rf::PlayerGetWeaponTotalAmmo(rf::local_player, weapon_cls_id) <= 0) {
            rf::GrSetColor(254, 55, 55, 255);
        }
        else {
            rf::GrSetColor(60, 126, 5, 255);
        }
        int idx = num_drawn_weapons_per_type[weapon_type];
        // indicator below type square
        int num_ind_w = g_big_weapon_cycle_hud ? 14 : 10;
        int num_ind_h = 3;
        int num_ind_delta_y = num_ind_h + 2;
        int num_ind_x = type_sq_start_x + sq_x_delta * weapon_type + sq_x_delta / 2 - num_ind_w / 2;
        int num_ind_y = num_ind_start_y + num_ind_delta_y * idx;
        rf::GrRect(num_ind_x, num_ind_y, num_ind_w, num_ind_h);

        if (weapon_type == selected_cycle_entry.type) {
            static int weapon_icon_bitmaps[32];
            static bool weapon_icon_bitmaps_initialized = false;
            if (!weapon_icon_bitmaps_initialized) {
                for (auto& bmh : weapon_icon_bitmaps) {
                    bmh = -1;
                }
                weapon_icon_bitmaps_initialized = true;
            }
            if (weapon_icon_bitmaps[weapon_cls_id] == -1) {
                auto weapon_icon = rf::weapon_classes[weapon_cls_id].weapon_icon.CStr();
                weapon_icon_bitmaps[weapon_cls_id] = rf::BmLoad(weapon_icon, -1, 1);
            }

            int weapon_icon_y = weapon_icons_start_y + weapon_icon_delta_y * idx;
            int weapon_icon_bmh = weapon_icon_bitmaps[weapon_cls_id];
            HudScaledBitmap(weapon_icon_bmh, weapon_icons_x, weapon_icon_y, weapon_icon_scale);
            if (weapon_cls_id == selected_cycle_entry.cls_id) {
                rf::GrSetColor(239, 213, 52, 255);
                HudRectBorder(weapon_icons_x - 1, weapon_icon_y - 1, weapon_icon_w + 2, weapon_icon_h + 2, 1);
            }
        }
        ++num_drawn_weapons_per_type[weapon_type];
    }
    rf::GrSetColor(15, 242, 2, 255);
    auto display_name = rf::weapon_classes[selected_cycle_entry.cls_id].display_name.CStr();
    int num_weapons_for_current_type = num_drawn_weapons_per_type[selected_cycle_entry.type];
    int weapon_name_y = weapon_icons_start_y + weapon_icon_delta_y * (num_weapons_for_current_type + 1);
    rf::GrStringAligned(rf::GR_ALIGN_CENTER, center_x, weapon_name_y, display_name, font_num);
}

FunHook RenderSelectWeaponGui_hook{0x004A2CF0, RenderSelectWeaponGui};

void InstallWeaponCycleHudPatches()
{
    RenderSelectWeaponGui_hook.Install();
}

void SetBigWeaponCycleHud(bool is_big)
{
    g_big_weapon_cycle_hud = is_big;
}
