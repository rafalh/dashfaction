#include "hud_internal.h"
#include "hud.h"
#include "../rf/hud.h"
#include "../rf/player/player.h"
#include "../rf/entity.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/weapon.h"
#include "../rf/localize.h"
#include "../rf/sound/sound.h"
#include <patch_common/FunHook.h>

bool weapon_select_big_mode = false;

void weapon_select_render()
{
    if (!rf::local_player) {
        return;
    }
    if (rf::hud_close_weapon_cycle_timer.elapsed() || !rf::hud_close_weapon_cycle_timer.valid()) {
        rf::snd_play(rf::hud_weapon_display_off_foley_snd, 0, 0.0f, 1.0f);
        rf::hud_render_weapon_cycle = false;
        return;
    }

    int clip_w = rf::gr::clip_width();
    rf::gr::set_color(15, 242, 2, 255);
    auto font_num = hud_get_default_font();

    // selected weapon type name
    auto& selected_cycle_entry = rf::hud_weapon_cycle[rf::hud_weapon_cycle_current_idx];
    const char* weapon_type_name = nullptr;
    switch (selected_cycle_entry.category) {
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
    int weapon_type_y = weapon_select_big_mode ? 210 : 113;
    int center_x = clip_w - (weapon_select_big_mode ? 148 : 74);
    if (weapon_type_name) {
        rf::gr::string_aligned(rf::gr::ALIGN_CENTER, center_x, weapon_type_y, weapon_type_name, font_num);
    }
    // weapon type squares
    int sq_bg_size = weapon_select_big_mode ? 30 : 20;
    int border = 1;
    int sq_spacing = 1;
    int sq_x_delta = sq_bg_size + 2 * border + sq_spacing; // 23
    int type_sq_y = weapon_type_y + (weapon_select_big_mode ? 24 : 14); // 127
    int type_sq_start_x = center_x - 2 * sq_x_delta; // w - 120
    int text_offset_x = sq_bg_size / 2;
    int text_offset_y = 5;

    // type squares - backgrounds
    rf::gr::set_color(0, 0, 0, 128);
    for (int i = 0; i < 4; ++i) {
        int sq_x = type_sq_start_x + i * sq_x_delta;
        rf::gr::rect(sq_x + border, type_sq_y + border + 1, sq_bg_size, sq_bg_size);
    }
    // type squares - labels
    rf::gr::set_color(15, 242, 2, 255);
    for (int i = 0; i < 4; ++i) {
        char digit_str[] = {static_cast<char>('1' + i), '\0'};
        int sq_x = type_sq_start_x + i * sq_x_delta;
        int sq_text_x = sq_x + border + text_offset_x;
        rf::gr::string_aligned(rf::gr::ALIGN_CENTER, sq_text_x, type_sq_y + text_offset_y, digit_str, font_num);
    }
    // borders for active type square
    int active_sq_x = type_sq_start_x + selected_cycle_entry.category * sq_x_delta;
    rf::gr::set_color(239, 213, 52, 255);
    int sq_with_border_size = sq_bg_size + 2 * border; // 22
    hud_rect_border(active_sq_x, type_sq_y, sq_with_border_size, sq_with_border_size, border);

    int num_ind_start_y = type_sq_y + sq_bg_size + 2 * border + sq_spacing + 1; // 150
    int weapon_icons_start_y = num_ind_start_y + 26; // 176
    float weapon_icon_scale = weapon_select_big_mode ? 1.5f : 1.0f;
    int weapon_icon_w = static_cast<int>(128 * weapon_icon_scale);
    int weapon_icon_h = static_cast<int>(34 * weapon_icon_scale);
    int weapon_icons_x = center_x - weapon_icon_w / 2; // clip_w - 139
    int weapon_icon_delta_y = weapon_icon_h + 1; // 35

    int num_drawn_weapons_per_category[4] = {0};
    rf::AiInfo* ai_info = rf::player_get_ai(rf::local_player);
    for (auto& pref_id : rf::local_player->weapon_prefs) {
        rf::WeaponCycle* cycle_entry = nullptr;
        for (auto& entry : rf::hud_weapon_cycle) {
            if (entry.weapon_type == pref_id) {
                cycle_entry = &entry;
            }
        }
        if (!cycle_entry || cycle_entry->weapon_type == -1 ||  cycle_entry->category == -1) {
            continue;
        }
        if (!rf::ai_has_weapon(ai_info, cycle_entry->weapon_type)) {
            continue;
        }
        int weapon_type = cycle_entry->weapon_type;
        int weapon_category = cycle_entry->category;
        if (rf::player_get_weapon_total_ammo(rf::local_player, weapon_type) <= 0) {
            rf::gr::set_color(254, 55, 55, 255);
        }
        else {
            rf::gr::set_color(60, 126, 5, 255);
        }
        int idx = num_drawn_weapons_per_category[weapon_category];
        // indicator below type square
        int num_ind_w = weapon_select_big_mode ? 14 : 10;
        int num_ind_h = 3;
        int num_ind_delta_y = num_ind_h + 2;
        int num_ind_x = type_sq_start_x + sq_x_delta * weapon_category + sq_x_delta / 2 - num_ind_w / 2;
        int num_ind_y = num_ind_start_y + num_ind_delta_y * idx;
        rf::gr::rect(num_ind_x, num_ind_y, num_ind_w, num_ind_h);

        if (weapon_category == selected_cycle_entry.category) {
            static int weapon_icon_bitmaps[32];
            static bool weapon_icon_bitmaps_initialized = false;
            if (!weapon_icon_bitmaps_initialized) {
                for (auto& bmh : weapon_icon_bitmaps) {
                    bmh = -1;
                }
                weapon_icon_bitmaps_initialized = true;
            }
            if (weapon_icon_bitmaps[weapon_type] == -1) {
                const char* weapon_icon = rf::weapon_types[weapon_type].weapon_select_icon_filename.c_str();
                weapon_icon_bitmaps[weapon_type] = rf::bm::load(weapon_icon, -1, false);
            }

            int weapon_icon_y = weapon_icons_start_y + weapon_icon_delta_y * idx;
            int weapon_icon_bmh = weapon_icon_bitmaps[weapon_type];
            hud_scaled_bitmap(weapon_icon_bmh, weapon_icons_x, weapon_icon_y, weapon_icon_scale);
            if (weapon_type == selected_cycle_entry.weapon_type) {
                rf::gr::set_color(239, 213, 52, 255);
                hud_rect_border(weapon_icons_x - 1, weapon_icon_y - 1, weapon_icon_w + 2, weapon_icon_h + 2, 1);
            }
        }
        ++num_drawn_weapons_per_category[weapon_category];
    }
    rf::gr::set_color(15, 242, 2, 255);
    auto& weapon_info = rf::weapon_types[selected_cycle_entry.weapon_type];
    const char* display_name = weapon_info.display_name.c_str();
    int num_weapons_for_current_type = num_drawn_weapons_per_category[selected_cycle_entry.category];
    int weapon_name_y = weapon_icons_start_y + weapon_icon_delta_y * (num_weapons_for_current_type + 1);
    rf::gr::string_aligned(rf::gr::ALIGN_CENTER, center_x, weapon_name_y, display_name, font_num);
}

FunHook weapon_select_render_hook{0x004A2CF0, weapon_select_render};

void weapon_select_apply_patches()
{
    weapon_select_render_hook.install();
}

void weapon_select_set_big(bool is_big)
{
    weapon_select_big_mode = is_big;
}
