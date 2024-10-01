#include <algorithm>
#include <format>
#include <common/utils/list-utils.h>
#include <patch_common/FunHook.h>
#include "multi_scoreboard.h"
#include "../multi/multi.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/multi.h"
#include "../rf/localize.h"
#include "../rf/entity.h"
#include "../rf/gameseq.h"
#include "../rf/hud.h"
#include "../rf/level.h"
#include "../rf/os/timer.h"
#include "../main/main.h"
#include "hud_internal.h"

#define DEBUG_SCOREBOARD 0

namespace rf
{
static auto& draw_scoreboard = addr_as_ref<void(bool draw)>(0x00470860);
static auto& fit_scoreboard_string = addr_as_ref<String* (String* result, String::Pod str, int cx_max)>(0x00471EC0);
}

constexpr float ENTER_ANIM_MS = 100.0f;
constexpr float LEAVE_ANIM_MS = 100.0f;

static bool g_scoreboard_force_hide = false;
static bool g_scoreboard_visible = false;
static unsigned g_anim_ticks = 0;
static bool g_enter_anim = false;
static bool g_leave_anim = false;
static bool g_big_scoreboard = false;

void multi_scoreboard_set_big(bool is_big)
{
    g_big_scoreboard = is_big;
}

int draw_scoreboard_header(int x, int y, int w, rf::NetGameType game_type, bool dry_run = false)
{
    // Draw RF logo
    int x_center = x + w / 2;
    int cur_y = y;
    if (!dry_run) {
        rf::gr::set_color(0xFF, 0xFF, 0xFF, 0xFF);
        static int score_rflogo_bm = rf::bm::load("score_rflogo.tga", -1, false);
        rf::gr::bitmap(score_rflogo_bm, x_center - 170, cur_y);
    }
    cur_y += 30;

    // Draw Game Type name
    if (!dry_run) {
        int num_players = rf::multi_num_players();
        std::string player_count_str =
            " | " + std::to_string(num_players) + (num_players > 1 ? " PLAYERS" : " PLAYER");

        std::string game_type_name = (game_type == rf::NG_TYPE_DM)    ? rf::strings::deathmatch
                                     : (game_type == rf::NG_TYPE_CTF) ? rf::strings::capture_the_flag
                                                                      : rf::strings::team_deathmatch;

        game_type_name += player_count_str;

        rf::gr::string_aligned(rf::gr::ALIGN_CENTER, x_center, cur_y, game_type_name.c_str());
    }
    int font_h = rf::gr::get_font_height(-1);
    cur_y += font_h + 8;

    // Draw level
    if (!dry_run) {
        rf::gr::set_color(0xB0, 0xB0, 0xB0, 0xFF);
        auto level_info = rf::String::format("{} ({}) by {}", rf::level.name, rf::level.filename, rf::level.author);
        rf::String level_info_stripped;
        rf::fit_scoreboard_string(&level_info_stripped, level_info, w - 20); // Note: this destroys input string
        rf::gr::string_aligned(rf::gr::ALIGN_CENTER, x_center, cur_y, level_info_stripped);
    }
    cur_y += font_h + 3;

    // Draw server info
    if (!dry_run) {
        char ip_addr_buf[64];
        rf::net_addr_to_string(ip_addr_buf, sizeof(ip_addr_buf), rf::netgame.server_addr);
        auto server_info = rf::String::format("{} ({})", rf::netgame.name, ip_addr_buf);
        rf::String server_info_stripped;
        rf::fit_scoreboard_string(&server_info_stripped, server_info, w - 20); // Note: this destroys input string
        rf::gr::string_aligned(rf::gr::ALIGN_CENTER, x_center, cur_y, server_info_stripped);
    }
    cur_y += font_h + 8;

    // Draw team scores
    if (game_type != rf::NG_TYPE_DM) {
        if (!dry_run) {
            unsigned red_score = 0, blue_score = 0;
            if (game_type == rf::NG_TYPE_CTF) {
                static int hud_flag_red_bm = rf::bm::load("hud_flag_red.tga", -1, true);
                static int hud_flag_blue_bm = rf::bm::load("hud_flag_blue.tga", -1, true);
                int flag_bm_w, flag_bm_h;
                rf::bm::get_dimensions(hud_flag_red_bm, &flag_bm_w, &flag_bm_h);
                rf::gr::bitmap(hud_flag_red_bm, x + w * 2 / 6 - flag_bm_w / 2, cur_y);
                rf::gr::bitmap(hud_flag_blue_bm, x + w * 4 / 6 - flag_bm_w / 2, cur_y);
                red_score = rf::multi_ctf_get_red_team_score();
                blue_score = rf::multi_ctf_get_blue_team_score();
            }
            else if (game_type == rf::NG_TYPE_TEAMDM) {
                red_score = rf::multi_tdm_get_red_team_score();
                blue_score = rf::multi_tdm_get_blue_team_score();
            }
            rf::gr::set_color(0xD0, 0x20, 0x20, 0xFF);
            int team_scores_font = rf::scoreboard_big_font;
            auto red_score_str = std::to_string(red_score);
            rf::gr::string_aligned(rf::gr::ALIGN_CENTER, x + w * 1 / 6, cur_y + 10, red_score_str.c_str(), team_scores_font);
            rf::gr::set_color(0x20, 0x20, 0xD0, 0xFF);
            auto blue_score_str = std::to_string(blue_score);
            rf::gr::string_aligned(rf::gr::ALIGN_CENTER, x + w * 5 / 6, cur_y + 10, blue_score_str.c_str(), team_scores_font);
        }

        cur_y += 60;
    }

    return cur_y - y;
}

int draw_scoreboard_players(const std::vector<rf::Player*>& players, int x, int y, int w, float scale,
    rf::NetGameType game_type, bool dry_run = false)
{
    int initial_y = y;
    int font_h = rf::gr::get_font_height(-1);

    int status_w = static_cast<int>(12 * scale);
    int score_w = static_cast<int>(50 * scale);
    int kd_w = static_cast<int>(70 * scale);
    int caps_w = game_type == rf::NG_TYPE_CTF ? static_cast<int>(45 * scale) : 0;
    int ping_w = static_cast<int>(35 * scale);
    int name_w = w - status_w - score_w - kd_w - caps_w - ping_w;

    int status_x = x;
    int name_x = status_x + status_w;
    int score_x = name_x + name_w;
    int kd_x = score_x + score_w;
    int caps_x = kd_x + kd_w;
    int ping_x = caps_x + caps_w;

    // Draw list header
    if (!dry_run) {
        rf::gr::set_color(0xFF, 0xFF, 0xFF, 0xFF);
        rf::gr::string(name_x, y, rf::strings::player);
        rf::gr::string(score_x, y, rf::strings::score); // Note: RF uses "Frags"
        rf::gr::string(kd_x, y, "K/D");
        if (game_type == rf::NG_TYPE_CTF) {
            rf::gr::string(caps_x, y, rf::strings::caps);
        }
        rf::gr::string(ping_x, y, rf::strings::ping, -1);
    }

    y += font_h + 8;

    rf::Player* red_flag_player = rf::multi_ctf_get_red_flag_player();
    rf::Player* blue_flag_player = rf::multi_ctf_get_blue_flag_player();

    // Draw the list
    for (rf::Player* player : players) {
        if (!dry_run) {
            bool is_local_player = player == rf::player_list;
            if (is_local_player)
                rf::gr::set_color(0xFF, 0xFF, 0x80, 0xFF);
            else
                rf::gr::set_color(0xFF, 0xFF, 0xFF, 0xFF);

            static int green_bm = rf::bm::load("DF_green.tga", -1, true);
            static int red_bm = rf::bm::load("DF_red.tga", -1, true);
            static int hud_micro_flag_red_bm = rf::bm::load("hud_microflag_red.tga", -1, true);
            static int hud_micro_flag_blue_bm = rf::bm::load("hud_microflag_blue.tga", -1, true);

            rf::Entity* entity = rf::entity_from_handle(player->entity_handle);
            int status_bm = entity ? green_bm : red_bm;
            if (player == red_flag_player)
                status_bm = hud_micro_flag_red_bm;
            else if (player == blue_flag_player)
                status_bm = hud_micro_flag_blue_bm;
            hud_scaled_bitmap(status_bm, status_x, static_cast<int>(y + 2 * scale), scale);

            rf::String player_name_stripped;
            rf::fit_scoreboard_string(&player_name_stripped, player->name, name_w - static_cast<int>(12 * scale)); // Note: this destroys Name
            rf::gr::string(name_x, y, player_name_stripped);

#if DEBUG_SCOREBOARD
            int score = 999;
            int num_kills = 999;
            int num_deaths = 999;
            int caps = 999;
            int ping = 9999;
#else
            auto* stats = static_cast<PlayerStatsNew*>(player->stats);
            int score = stats->score;
            int num_kills = stats->num_kills;
            int num_deaths = stats->num_deaths;
            int caps = stats->caps;
            int ping = player->net_data ? player->net_data->ping : 0;
#endif

            auto score_str = std::to_string(score);
            rf::gr::string(score_x, y, score_str.c_str());

            auto kills_deaths_str = std::format("{}/{}", num_kills, num_deaths);
            rf::gr::string(kd_x, y, kills_deaths_str.c_str());

            if (game_type == rf::NG_TYPE_CTF) {
                auto caps_str = std::to_string(caps);
                rf::gr::string(caps_x, y, caps_str.c_str());
            }

            auto ping_str = std::to_string(ping);
            rf::gr::string(ping_x, y, ping_str.c_str());
        }

        y += font_h + (scale == 1.0f ? 3 : 0);
    }

    return y - initial_y;
}

void filter_and_sort_players(std::vector<rf::Player*>& players, std::optional<int> team_id)
{
    players.clear();
    players.reserve(32);

    auto player_list = SinglyLinkedList{rf::player_list};
    for (auto& player : player_list) {
        if (!team_id || player.team == team_id.value())
            players.push_back(&player);
    }
    std::sort(players.begin(), players.end(), [](auto player1, auto player2) {
        return player1->stats->score > player2->stats->score;
    });
}

void draw_scoreboard_internal_new(bool draw)
{
    if (g_scoreboard_force_hide || !draw)
        return;

    auto game_type = rf::multi_get_game_type();
    std::vector<rf::Player*> left_players, right_players;
#if DEBUG_SCOREBOARD
    for (int i = 0; i < 24; ++i) {
        left_players.push_back(rf::local_player);
    }
    for (int i = 0; i < 8; ++i) {
        right_players.push_back(rf::local_player);
    }
    game_type = rf::NG_TYPE_CTF;
    bool group_by_team = game_type != rf::NG_TYPE_DM;
#else
    // Sort players by score
    bool group_by_team = game_type != rf::NG_TYPE_DM;
    if (group_by_team) {
        filter_and_sort_players(left_players, {rf::TEAM_RED});
        filter_and_sort_players(right_players, {rf::TEAM_BLUE});
    }
    else {
        filter_and_sort_players(left_players, {});
    }
#endif

    // Animation
    float anim_progress = 1.0f;
    float progress_w = 1.0f;
    float progress_h = 1.0f;
    if (g_game_config.scoreboard_anim) {
        unsigned anim_delta = rf::timer_get(1000) - g_anim_ticks;
        if (g_enter_anim)
            anim_progress = anim_delta / ENTER_ANIM_MS;
        else if (g_leave_anim)
            anim_progress = (LEAVE_ANIM_MS - anim_delta) / LEAVE_ANIM_MS;

        if (g_leave_anim && anim_progress <= 0.0f) {
            g_scoreboard_visible = false;
            return;
        }

        progress_w = anim_progress * 2.0f;
        progress_h = (anim_progress - 0.5f) * 2.0f;

        progress_w = std::clamp(progress_w, 0.1f, 1.0f);
        progress_h = std::clamp(progress_h, 0.1f, 1.0f);
    }

    int w;
    float scale;
    // Note: fit_scoreboard_string does not support providing font by argument so default font must be changed
    if (g_big_scoreboard) {
        rf::gr::set_default_font(hud_get_default_font_name(true));
        w = std::min(!group_by_team ? 900 : 1400, rf::gr::clip_width());
        scale = 2.0f;
    }
    else {
        w = std::min(!group_by_team ? 450 : 700, rf::gr::clip_width());
        scale = 1.0f;
    }

    int left_padding = static_cast<int>(10 * scale);
    int right_padding = static_cast<int>(10 * scale);
    int middle_padding = static_cast<int>(15 * scale);
    int top_padding = static_cast<int>(10 * scale);
    int bottom_padding = static_cast<int>(5 * scale);
    int hdr_h = draw_scoreboard_header(0, 0, w, game_type, true);
    int left_players_h = draw_scoreboard_players(left_players, 0, 0, 0, scale, game_type, true);
    int right_players_h = draw_scoreboard_players(right_players, 0, 0, 0, scale, game_type, true);
    int h = top_padding + hdr_h + std::max(left_players_h, right_players_h) + bottom_padding;

    // Draw background
    w = static_cast<int>(progress_w * w);
    h = static_cast<int>(progress_h * h);
    int x = (static_cast<int>(rf::gr::clip_width()) - w) / 2;
    int y = (static_cast<int>(rf::gr::clip_height()) - h) / 2;
    rf::gr::set_color(0, 0, 0, 0x80);
    rf::gr::rect(x, y, w, h);
    y += top_padding;

    if (progress_h < 1.0f || progress_w < 1.0f) {
        // Restore rfpc-medium as default font
        if (g_big_scoreboard) {
            rf::gr::set_default_font("rfpc-medium.vf");
        }
        return;
    }

    y += draw_scoreboard_header(x, y, w, game_type);
    if (group_by_team) {
        int table_w = (w - left_padding - middle_padding - right_padding) / 2;
        draw_scoreboard_players(left_players, x + left_padding, y, table_w, scale, game_type);
        draw_scoreboard_players(right_players, x + left_padding + table_w + middle_padding, y, table_w, scale, game_type);
    }
    else {
        int table_w = w - left_padding - right_padding;
        draw_scoreboard_players(left_players, x + left_padding, y, table_w, scale, game_type);
    }

    // Restore rfpc-medium as default font
    if (g_big_scoreboard) {
        rf::gr::set_default_font("rfpc-medium.vf");
    }
}

FunHook<void(bool)> draw_scoreboard_internal_hook{0x00470880, draw_scoreboard_internal_new};

void scoreboard_maybe_render(bool show_scoreboard)
{
    if (g_game_config.scoreboard_anim) {
        if (!g_scoreboard_visible && show_scoreboard) {
            g_enter_anim = true;
            g_leave_anim = false;
            g_anim_ticks = rf::timer_get(1000);
            g_scoreboard_visible = true;
        }
        if (g_scoreboard_visible && !show_scoreboard && !g_leave_anim) {
            g_enter_anim = false;
            g_leave_anim = true;
            g_anim_ticks = rf::timer_get(1000);
        }
    }
    else {
        g_scoreboard_visible = show_scoreboard;
    }

    if (g_scoreboard_visible) {
        rf::draw_scoreboard(true);
    }
}

void multi_scoreboard_apply_patch()
{
    draw_scoreboard_internal_hook.install();
}

void multi_scoreboard_set_hidden(bool hidden)
{
    g_scoreboard_force_hide = hidden;
}
