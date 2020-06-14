#include <patch_common/AsmWriter.h>
#include "../rf/hud.h"
#include "../rf/graphics.h"
#include "../rf/network.h"
#include "../rf/player.h"
#include "hud_internal.h"
#include "hud.h"

static bool g_big_team_scores_hud = false;
constexpr bool g_debug_team_scores_hud = false;

namespace rf
{
    auto& hud_miniflag_red_bmh = AddrAsRef<int>(0x0059DF48);
    auto& hud_miniflag_blue_bmh = AddrAsRef<int>(0x0059DF4C);
    auto& hud_miniflag_hilight_bmh = AddrAsRef<int>(0x0059DF50);
    auto& hud_flag_red_bmh = AddrAsRef<int>(0x0059DF54);
    auto& hud_flag_blue_bmh = AddrAsRef<int>(0x0059DF58);
    auto& hud_flag_render_state = AddrAsRef<rf::GrRenderState>(0x01775B30); //MatT2C2A3B2Z1F3
}

void HudRenderTeamScores()
{
    int clip_h = rf::GrGetClipHeight();
    rf::GrSetColor(0, 0, 0, 150);
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

    rf::GrRect(10, clip_h - box_h - 10, box_w, box_h);
    auto game_type = rf::MultiGetGameType();
    int font_id = HudGetDefaultFont();

    if (game_type == rf::MGT_CTF) {
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
        rf::GrSetColor(53, 207, 22, 255);
        auto red_flag_player = g_debug_team_scores_hud ? rf::local_player : rf::CtfGetRedFlagPlayer();
        if (red_flag_player) {
            auto name = g_debug_team_scores_hud ? "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii" : red_flag_player->name;
            std::string fitting_name = HudFitString(name, max_miniflag_label_w, nullptr, font_id);
            rf::GrString(miniflag_label_x, red_miniflag_label_y, fitting_name.c_str(), font_id);

            if (red_flag_player == rf::local_player) {
                rf::GrSetColor(255, 255, 255, static_cast<int>(hud_flag_alpha));
                HudScaledBitmap(rf::hud_flag_red_bmh, flag_x, box_y, flag_scale, rf::hud_flag_render_state);
            }
        }
        else if (rf::CtfIsRedFlagInBase()) {
            rf::GrString(miniflag_label_x, red_miniflag_label_y, "at base", font_id);
        }
        else {
            rf::GrString(miniflag_label_x, red_miniflag_label_y, "missing", font_id);
        }
        rf::GrSetColor(53, 207, 22, 255);
        auto blue_flag_player = rf::CtfGetBlueFlagPlayer();
        if (blue_flag_player) {
            auto name = g_debug_team_scores_hud ? "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii" : blue_flag_player->name;
            std::string fitting_name = HudFitString(name, max_miniflag_label_w, nullptr, font_id);
            rf::GrString(miniflag_label_x, blue_miniflag_label_y, fitting_name.c_str(), font_id);

            if (blue_flag_player == rf::local_player) {
                rf::GrSetColor(255, 255, 255, static_cast<int>(hud_flag_alpha));
                HudScaledBitmap(rf::hud_flag_blue_bmh, flag_x, box_y, flag_scale, rf::hud_flag_render_state);
            }
        }
        else if (rf::CtfIsBlueFlagInBase()) {
            rf::GrString(miniflag_label_x, blue_miniflag_label_y, "at base", font_id);
        }
        else {
            rf::GrString(miniflag_label_x, blue_miniflag_label_y, "missing", font_id);
        }

        float miniflag_scale = g_big_team_scores_hud ? 1.5f : 1.0f;
        rf::GrSetColor(255, 255, 255, 255);
        if (rf::local_player) {
            int miniflag_hilight_y;
            if (rf::local_player->team == rf::TEAM_RED) {
                miniflag_hilight_y = red_miniflag_y;
            }
            else {
                miniflag_hilight_y = blue_miniflag_y;
            }
            HudScaledBitmap(rf::hud_miniflag_hilight_bmh, miniflag_x, miniflag_hilight_y, miniflag_scale, rf::hud_flag_render_state);
        }
        HudScaledBitmap(rf::hud_miniflag_red_bmh, miniflag_x, red_miniflag_y, miniflag_scale, rf::hud_flag_render_state);
        HudScaledBitmap(rf::hud_miniflag_blue_bmh, miniflag_x, blue_miniflag_y, miniflag_scale, rf::hud_flag_render_state);
    }

    int red_score, blue_score;
    if (g_debug_team_scores_hud) {
        red_score = 15;
        blue_score = 15;
    }
    else if (game_type == rf::MGT_CTF) {
        red_score = rf::CtfGetRedTeamScore();
        blue_score = rf::CtfGetBlueTeamScore();
    }
    else if (game_type == rf::MGT_TEAMDM) {
        rf::GrSetColor(53, 207, 22, 255);
        red_score = rf::TdmGetRedTeamScore();
        blue_score = rf::TdmGetBlueTeamScore();
    }
    else {
        red_score = 0;
        blue_score = 0;
    }
    auto red_score_str = std::to_string(red_score);
    auto blue_score_str = std::to_string(blue_score);
    int str_w, str_h;
    rf::GrGetTextWidth(&str_w, &str_h, red_score_str.c_str(), -1, font_id);
    rf::GrString(box_x + box_w - 5 - str_w, red_miniflag_label_y, red_score_str.c_str(), font_id);
    rf::GrGetTextWidth(&str_w, &str_h, blue_score_str.c_str(), -1, font_id);
    rf::GrString(box_x + box_w - 5 - str_w, blue_miniflag_label_y, blue_score_str.c_str(), font_id);
}

void ApplyTeamScoresHudPatches()
{
    AsmWriter{0x00477790}.jmp(HudRenderTeamScores);
}

void SetBigTeamScoresHud(bool is_big)
{
    g_big_team_scores_hud = is_big;
}
