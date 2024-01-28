#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include "../rf/hud.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/multi.h"
#include "../rf/player/player.h"
#include "../rf/os/frametime.h"
#include "../main/main.h"
#include "../graphics/gr.h"
#include "hud_internal.h"
#include "hud.h"

static bool g_big_team_scores_hud = false;
constexpr bool g_debug_team_scores_hud = false;

namespace rf
{
    auto& hud_miniflag_red_bmh = addr_as_ref<int>(0x0059DF48);
    auto& hud_miniflag_blue_bmh = addr_as_ref<int>(0x0059DF4C);
    auto& hud_miniflag_hilight_bmh = addr_as_ref<int>(0x0059DF50);
    auto& hud_flag_red_bmh = addr_as_ref<int>(0x0059DF54);
    auto& hud_flag_blue_bmh = addr_as_ref<int>(0x0059DF58);
    auto& hud_flag_gr_mode = addr_as_ref<rf::gr::Mode>(0x01775B30);
}

void hud_render_team_scores()
{
    int clip_h = rf::gr::clip_height();
    rf::gr::set_color(0, 0, 0, 150);
    int box_w = g_big_team_scores_hud ? 370 : 185;
    int box_h = g_big_team_scores_hud ? 80 : 55;
    int box_x = 10;
    int box_y = clip_h - box_h - 10; // clip_h - 65
    int miniflag_x = box_x + 7; // 17
    int miniflag_label_x = box_x + (g_big_team_scores_hud ? 45 : 33); // 43
    int max_miniflag_label_w = box_w - (g_big_team_scores_hud ? 80 : 55);
    int red_miniflag_y = box_y + 4; // clip_h - 61
    int blue_miniflag_y = box_y + (g_big_team_scores_hud ? 42 : 30); // clip_h - 35
    int red_miniflag_label_y = red_miniflag_y + 4; // clip_h - 57
    int blue_miniflag_label_y = blue_miniflag_y + 4; // clip_h - 31
    int flag_x = g_big_team_scores_hud ? 410 : 205;
    float flag_scale = g_big_team_scores_hud ? 1.5f : 1.0f;

    rf::gr::rect(10, clip_h - box_h - 10, box_w, box_h);
    auto game_type = rf::multi_get_game_type();
    int font_id = hud_get_default_font();

    if (game_type == rf::NG_TYPE_CTF) {
        static float hud_flag_alpha = 255.0f;
        static bool hud_flag_pulse_dir = false;
        float delta_alpha = rf::frametime * 500.0f;
        if (hud_flag_pulse_dir) {
            hud_flag_alpha -= delta_alpha;
            if (hud_flag_alpha <= 50.0f) {
                hud_flag_alpha = 50.0f;
                hud_flag_pulse_dir = false;
            }
        }
        else {
            hud_flag_alpha += delta_alpha;
            if (hud_flag_alpha >= 255.0f) {
                hud_flag_alpha = 255.0f;
                hud_flag_pulse_dir = true;
            }
        }
        rf::gr::set_color(53, 207, 22, 255);
        rf::Player* red_flag_player = g_debug_team_scores_hud ? rf::local_player : rf::multi_ctf_get_red_flag_player();
        if (red_flag_player) {
            const char* name = red_flag_player->name;
            std::string fitting_name = hud_fit_string(name, max_miniflag_label_w, nullptr, font_id);
            rf::gr::string(miniflag_label_x, red_miniflag_label_y, fitting_name.c_str(), font_id);

            if (red_flag_player == rf::local_player) {
                rf::gr::set_color(255, 255, 255, static_cast<int>(hud_flag_alpha));
                hud_scaled_bitmap(rf::hud_flag_red_bmh, flag_x, box_y, flag_scale, rf::hud_flag_gr_mode);
            }
        }
        else if (rf::multi_ctf_is_red_flag_in_base()) {
            rf::gr::string(miniflag_label_x, red_miniflag_label_y, "at base", font_id);
        }
        else {
            rf::gr::string(miniflag_label_x, red_miniflag_label_y, "missing", font_id);
        }
        rf::gr::set_color(53, 207, 22, 255);
        rf::Player* blue_flag_player = rf::multi_ctf_get_blue_flag_player();
        if (blue_flag_player) {
            const char* name = blue_flag_player->name;
            std::string fitting_name = hud_fit_string(name, max_miniflag_label_w, nullptr, font_id);
            rf::gr::string(miniflag_label_x, blue_miniflag_label_y, fitting_name.c_str(), font_id);

            if (blue_flag_player == rf::local_player) {
                rf::gr::set_color(255, 255, 255, static_cast<int>(hud_flag_alpha));
                hud_scaled_bitmap(rf::hud_flag_blue_bmh, flag_x, box_y, flag_scale, rf::hud_flag_gr_mode);
            }
        }
        else if (rf::multi_ctf_is_blue_flag_in_base()) {
            rf::gr::string(miniflag_label_x, blue_miniflag_label_y, "at base", font_id);
        }
        else {
            rf::gr::string(miniflag_label_x, blue_miniflag_label_y, "missing", font_id);
        }
    }

    if (game_type == rf::NG_TYPE_CTF || game_type == rf::NG_TYPE_TEAMDM) {
        float miniflag_scale = g_big_team_scores_hud ? 1.5f : 1.0f;
        rf::gr::set_color(255, 255, 255, 255);
        if (rf::local_player) {
            int miniflag_hilight_y;
            if (rf::local_player->team == rf::TEAM_RED) {
                miniflag_hilight_y = red_miniflag_y;
            }
            else {
                miniflag_hilight_y = blue_miniflag_y;
            }
            hud_scaled_bitmap(rf::hud_miniflag_hilight_bmh, miniflag_x, miniflag_hilight_y, miniflag_scale, rf::hud_flag_gr_mode);
        }
        hud_scaled_bitmap(rf::hud_miniflag_red_bmh, miniflag_x, red_miniflag_y, miniflag_scale, rf::hud_flag_gr_mode);
        hud_scaled_bitmap(rf::hud_miniflag_blue_bmh, miniflag_x, blue_miniflag_y, miniflag_scale, rf::hud_flag_gr_mode);
    }

    int red_score, blue_score;
    if (g_debug_team_scores_hud) {
        red_score = 15;
        blue_score = 15;
    }
    else if (game_type == rf::NG_TYPE_CTF) {
        red_score = rf::multi_ctf_get_red_team_score();
        blue_score = rf::multi_ctf_get_blue_team_score();
    }
    else if (game_type == rf::NG_TYPE_TEAMDM) {
        rf::gr::set_color(53, 207, 22, 255);
        red_score = rf::multi_tdm_get_red_team_score();
        blue_score = rf::multi_tdm_get_blue_team_score();
    }
    else {
        red_score = 0;
        blue_score = 0;
    }
    auto red_score_str = std::to_string(red_score);
    auto blue_score_str = std::to_string(blue_score);
    int str_w, str_h;
    rf::gr::get_string_size(&str_w, &str_h, red_score_str.c_str(), -1, font_id);
    rf::gr::string(box_x + box_w - 5 - str_w, red_miniflag_label_y, red_score_str.c_str(), font_id);
    rf::gr::get_string_size(&str_w, &str_h, blue_score_str.c_str(), -1, font_id);
    rf::gr::string(box_x + box_w - 5 - str_w, blue_miniflag_label_y, blue_score_str.c_str(), font_id);
}

CallHook<void(int, int, int, rf::gr::Mode)> hud_render_power_ups_gr_bitmap_hook{
    {
        0x0047FF2F,
        0x0047FF96,
        0x0047FFFD,
    },
    [](int bm_handle, int x, int y, rf::gr::Mode mode) {
        float scale = g_game_config.big_hud ? 2.0f : 1.0f;
        x = hud_transform_value(x, 640, rf::gr::clip_width());
        x = hud_scale_value(x, rf::gr::clip_width(), scale);
        y = hud_scale_value(y, rf::gr::clip_height(), scale);
        hud_scaled_bitmap(bm_handle, x, y, scale, mode);
    },
};

FunHook<void()> render_level_info_hook{
    0x00477180,
    []() {
        gr_font_run_with_default(hud_get_default_font(), [&]() {
            render_level_info_hook.call_target();
        });
    },
};

FunHook<void()> multi_hud_init_hook{
    0x00476AD0,
    []() {
        // Change font for Time Left text
        static int time_left_font = rf::gr::load_font("rfpc-large.vf");
        if (time_left_font >= 0) {
            write_mem<i8>(0x00477157 + 1, time_left_font);
        }
    },
};

void multi_hud_apply_patches()
{
    AsmWriter{0x00477790}.jmp(hud_render_team_scores);
    hud_render_power_ups_gr_bitmap_hook.install();
    render_level_info_hook.install();
    multi_hud_init_hook.install();

    // Change position of Time Left text
    write_mem<i8>(0x0047715F + 2, 21);
    write_mem<i32>(0x00477168 + 1, 154);
}

void multi_hud_set_big(bool is_big)
{
    g_big_team_scores_hud = is_big;
}
