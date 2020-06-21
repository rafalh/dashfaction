#include "scoreboard.h"
#include "../multi/kill.h"
#include "../rf/graphics.h"
#include "../rf/network.h"
#include "../rf/misc.h"
#include "../rf/entity.h"
#include "../rf/game_seq.h"
#include "../rf/hud.h"
#include "../utils/list-utils.h"
#include "../main.h"
#include "spectate_mode.h"
#include "hud_internal.h"
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <algorithm>

#define DEBUG_SCOREBOARD 0

namespace rf
{
static auto& DrawScoreboard = AddrAsRef<void(bool draw)>(0x00470860);
static auto& FitScoreboardString = AddrAsRef<String* (String* result, String::Pod str, int cx_max)>(0x00471EC0);
}

constexpr float ENTER_ANIM_MS = 100.0f;
constexpr float LEAVE_ANIM_MS = 100.0f;
constexpr float HALF_ENTER_ANIM_MS = ENTER_ANIM_MS / 2.0f;
constexpr float HALF_LEAVE_ANIM_MS = LEAVE_ANIM_MS / 2.0f;

static bool g_scoreboard_force_hide = false;
static bool g_scoreboard_visible = false;
static unsigned g_anim_ticks = 0;
static bool g_enter_anim = false;
static bool g_leave_anim = false;
static bool g_big_scoreboard = false;

void SetBigScoreboard(bool is_big)
{
    g_big_scoreboard = is_big;
}

int DrawScoreboardHeader(int x, int y, int w, rf::MultiGameType game_type, bool dry_run = false)
{
    // Draw RF logo
    int x_center = x + w / 2;
    int cur_y = y;
    if (!dry_run) {
        rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        static int score_rflogo_bm = rf::BmLoad("score_rflogo.tga", -1, false);
        rf::GrBitmap(score_rflogo_bm, x_center - 170, cur_y);
    }
    cur_y += 30;

    // Draw Game Type name
    if (!dry_run) {
        const char* game_type_name;
        if (game_type == rf::MGT_DM)
            game_type_name = rf::strings::deathmatch;
        else if (game_type == rf::MGT_CTF)
            game_type_name = rf::strings::capture_the_flag;
        else
            game_type_name = rf::strings::team_deathmatch;
        rf::GrStringAligned(rf::GR_ALIGN_CENTER, x_center, cur_y, game_type_name);
    }
    int font_h = rf::GrGetFontHeight(-1);
    cur_y += font_h + 8;

    // Draw level
    if (!dry_run) {
        rf::GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
        auto level_info = rf::String::Format("%s (%s) by %s", rf::level_name.CStr(), rf::level_filename.CStr(),
                                            rf::level_author.CStr());
        rf::String level_info_stripped;
        rf::FitScoreboardString(&level_info_stripped, level_info, w - 20); // Note: this destroys input string
        rf::GrStringAligned(rf::GR_ALIGN_CENTER, x_center, cur_y, level_info_stripped);
    }
    cur_y += font_h + 3;

    // Draw server info
    if (!dry_run) {
        char ip_addr_buf[64];
        rf::NwAddrToStr(ip_addr_buf, sizeof(ip_addr_buf), rf::serv_addr);
        auto server_info = rf::String::Format("%s (%s)", rf::serv_name.CStr(), ip_addr_buf);
        rf::String server_info_stripped;
        rf::FitScoreboardString(&server_info_stripped, server_info, w - 20); // Note: this destroys input string
        rf::GrStringAligned(rf::GR_ALIGN_CENTER, x_center, cur_y, server_info_stripped);
    }
    cur_y += font_h + 8;

    // Draw team scores
    if (game_type != rf::MGT_DM) {
        if (!dry_run) {
            unsigned red_score = 0, blue_score = 0;
            if (game_type == rf::MGT_CTF) {
                static int hud_flag_red_bm = rf::BmLoad("hud_flag_red.tga", -1, true);
                static int hud_flag_blue_bm = rf::BmLoad("hud_flag_blue.tga", -1, true);
                int flag_bm_w, flag_bm_h;
                rf::BmGetBitmapSize(hud_flag_red_bm, &flag_bm_w, &flag_bm_h);
                rf::GrBitmap(hud_flag_red_bm, x + w * 2 / 6 - flag_bm_w / 2, cur_y);
                rf::GrBitmap(hud_flag_blue_bm, x + w * 4 / 6 - flag_bm_w / 2, cur_y);
                red_score = rf::CtfGetRedTeamScore();
                blue_score = rf::CtfGetBlueTeamScore();
            }
            else if (game_type == rf::MGT_TEAMDM) {
                red_score = rf::TdmGetRedTeamScore();
                blue_score = rf::TdmGetBlueTeamScore();
            }
            rf::GrSetColor(0xD0, 0x20, 0x20, 0xFF);
            int team_scores_font = rf::scoreboard_big_font;
            auto red_score_str = std::to_string(red_score);
            rf::GrStringAligned(rf::GR_ALIGN_CENTER, x + w * 1 / 6, cur_y + 10, red_score_str.c_str(), team_scores_font);
            rf::GrSetColor(0x20, 0x20, 0xD0, 0xFF);
            auto blue_score_str = std::to_string(blue_score);
            rf::GrStringAligned(rf::GR_ALIGN_CENTER, x + w * 5 / 6, cur_y + 10, blue_score_str.c_str(), team_scores_font);
        }

        cur_y += 60;
    }

    return cur_y - y;
}

int DrawScoreboardPlayers(const std::vector<rf::Player*>& players, int x, int y, int w, float scale,
    rf::MultiGameType game_type, bool dry_run = false)
{
    int initial_y = y;
    int font_h = rf::GrGetFontHeight(-1);

    int status_w = static_cast<int>(12 * scale);
    int score_w = static_cast<int>(50 * scale);
    int kd_w = static_cast<int>(70 * scale);
    int caps_w = game_type == rf::MGT_CTF ? static_cast<int>(45 * scale) : 0;
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
        rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        rf::GrString(name_x, y, rf::strings::player);
        rf::GrString(score_x, y, rf::strings::score); // Note: RF uses "Frags"
        rf::GrString(kd_x, y, "K/D");
        if (game_type == rf::MGT_CTF) {
            rf::GrString(caps_x, y, rf::strings::caps);
        }
        rf::GrString(ping_x, y, rf::strings::ping, -1);
    }

    y += font_h + 8;

    rf::Player* red_flag_player = rf::CtfGetRedFlagPlayer();
    rf::Player* blue_flag_player = rf::CtfGetBlueFlagPlayer();

    // Draw the list
    for (const auto player : players) {
        if (!dry_run) {
            bool is_local_player = player == rf::player_list;
            if (is_local_player)
                rf::GrSetColor(0xFF, 0xFF, 0x80, 0xFF);
            else
                rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);

            static int green_bm = rf::BmLoad("DF_green.tga", -1, true);
            static int red_bm = rf::BmLoad("DF_red.tga", -1, true);
            static int hud_micro_flag_red_bm = rf::BmLoad("hud_microflag_red.tga", -1, true);
            static int hud_micro_flag_blue_bm = rf::BmLoad("hud_microflag_blue.tga", -1, true);

            rf::EntityObj* entity = rf::EntityGetByHandle(player->entity_handle);
            int status_bm = entity ? green_bm : red_bm;
            if (player == red_flag_player)
                status_bm = hud_micro_flag_red_bm;
            else if (player == blue_flag_player)
                status_bm = hud_micro_flag_blue_bm;
            HudScaledBitmap(status_bm, status_x, static_cast<int>(y + 2 * scale), scale);

            rf::String player_name_stripped;
            rf::FitScoreboardString(&player_name_stripped, player->name, name_w - static_cast<int>(12 * scale)); // Note: this destroys Name
            rf::GrString(name_x, y, player_name_stripped);

#if DEBUG_SCOREBOARD
            int score = 999;
            int num_kills = 999;
            int num_deaths = 999;
            int caps = 999;
            int ping = 9999;
#else
            auto stats = static_cast<PlayerStatsNew*>(player->stats);
            int score = stats->score;
            int num_kills = stats->num_kills;
            int num_deaths = stats->num_deaths;
            int caps = stats->caps;
            int ping = player->nw_data ? player->nw_data->ping : 0;
#endif

            auto score_str = std::to_string(score);
            rf::GrString(score_x, y, score_str.c_str());

            auto kills_deaths_str = StringFormat("%hd/%hd", num_kills, num_deaths);
            rf::GrString(kd_x, y, kills_deaths_str.c_str());

            if (game_type == rf::MGT_CTF) {
                auto caps_str = std::to_string(caps);
                rf::GrString(caps_x, y, caps_str.c_str());
            }

            auto ping_str = std::to_string(ping);
            rf::GrString(ping_x, y, ping_str.c_str());
        }

        y += font_h + (scale == 1.0f ? 3 : 0);
    }

    return y - initial_y;
}

void FilterAndSortPlayers(std::vector<rf::Player*>& players, std::optional<int> team_id)
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

void DrawScoreboardInternal_New(bool draw)
{
    if (g_scoreboard_force_hide || !draw)
        return;

    auto game_type = rf::MultiGetGameType();
    std::vector<rf::Player*> left_players, right_players;
#if DEBUG_SCOREBOARD
    for (int i = 0; i < 24; ++i) {
        left_players.push_back(rf::local_player);
    }
    for (int i = 0; i < 8; ++i) {
        right_players.push_back(rf::local_player);
    }
    game_type = rf::MGT_CTF;
    bool group_by_team = game_type != rf::MGT_DM;
#else
    // Sort players by score
    bool group_by_team = game_type != rf::MGT_DM;
    if (group_by_team) {
        FilterAndSortPlayers(left_players, {rf::TEAM_RED});
        FilterAndSortPlayers(right_players, {rf::TEAM_BLUE});
    }
    else {
        FilterAndSortPlayers(left_players, {});
    }
#endif

    // Animation
    float anim_progress = 1.0f, progress_w = 1.0f, progress_h = 1.0f;
    if (g_game_config.scoreboard_anim) {
        unsigned anim_delta = rf::TimerGet(1000) - g_anim_ticks;
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
    // Note: FitScoreboardString does not support providing font by argument so default font must be changed
    if (g_big_scoreboard) {
        rf::GrSetDefaultFont(HudGetDefaultFontName(true));
        w = std::min(!group_by_team ? 900 : 1400, rf::GrGetClipWidth());
        scale = 2.0f;
    }
    else {
        w = std::min(!group_by_team ? 450 : 700, rf::GrGetClipWidth());
        scale = 1.0f;
    }

    int left_padding = static_cast<int>(10 * scale);
    int right_padding = static_cast<int>(10 * scale);
    int middle_padding = static_cast<int>(15 * scale);
    int top_padding = static_cast<int>(10 * scale);
    int bottom_padding = static_cast<int>(5 * scale);
    int hdr_h = DrawScoreboardHeader(0, 0, w, game_type, true);
    int left_players_h = DrawScoreboardPlayers(left_players, 0, 0, 0, scale, game_type, true);
    int right_players_h = DrawScoreboardPlayers(right_players, 0, 0, 0, scale, game_type, true);
    int h = top_padding + hdr_h + std::max(left_players_h, right_players_h) + bottom_padding;

    // Draw background
    w = static_cast<int>(progress_w * w);
    h = static_cast<int>(progress_h * h);
    int x = (static_cast<int>(rf::GrGetClipWidth()) - w) / 2;
    int y = (static_cast<int>(rf::GrGetClipHeight()) - h) / 2;
    rf::GrSetColor(0, 0, 0, 0x80);
    rf::GrRect(x, y, w, h);
    y += top_padding;

    if (progress_h < 1.0f || progress_w < 1.0f) {
        // Restore rfpc-medium as default font
        if (g_big_scoreboard) {
            rf::GrSetDefaultFont("rfpc-medium.vf");
        }
        return;
    }

    y += DrawScoreboardHeader(x, y, w, game_type);
    if (group_by_team) {
        int table_w = (w - left_padding - middle_padding - right_padding) / 2;
        DrawScoreboardPlayers(left_players, x + left_padding, y, table_w, scale, game_type);
        DrawScoreboardPlayers(right_players, x + left_padding + table_w + middle_padding, y, table_w, scale, game_type);
    }
    else {
        int table_w = w - left_padding - right_padding;
        DrawScoreboardPlayers(left_players, x + left_padding, y, table_w, scale, game_type);
    }

    // Restore rfpc-medium as default font
    if (g_big_scoreboard) {
        rf::GrSetDefaultFont("rfpc-medium.vf");
    }
}

FunHook<void(bool)> DrawScoreboardInternal_hook{0x00470880, DrawScoreboardInternal_New};

void HudRender_00437BC0()
{
#if DEBUG_SCOREBOARD
    rf::DrawScoreboard(true);
#else
    if (!rf::is_multi || !rf::local_player)
        return;

    bool scoreboard_control_active = rf::IsEntityCtrlActive(&rf::local_player->config.controls, rf::GC_MP_STATS, 0);
    bool is_player_dead = rf::IsPlayerEntityInvalid(rf::local_player) || rf::IsPlayerDying(rf::local_player);
    bool limbo = rf::GameSeqGetState() == rf::GS_MULTI_LIMBO;
    bool show_scoreboard = scoreboard_control_active || (!SpectateModeIsActive() && is_player_dead) || limbo;

    if (g_game_config.scoreboard_anim) {
        if (!g_scoreboard_visible && show_scoreboard) {
            g_enter_anim = true;
            g_leave_anim = false;
            g_anim_ticks = rf::TimerGet(1000);
            g_scoreboard_visible = true;
        }
        if (g_scoreboard_visible && !show_scoreboard && !g_leave_anim) {
            g_enter_anim = false;
            g_leave_anim = true;
            g_anim_ticks = rf::TimerGet(1000);
        }
    }
    else
        g_scoreboard_visible = show_scoreboard;

    if (g_scoreboard_visible)
        rf::DrawScoreboard(true);
#endif
}

void InitScoreboard()
{
    DrawScoreboardInternal_hook.Install();

    AsmWriter(0x00437BC0).call(HudRender_00437BC0).jmp(0x00437C24);
    AsmWriter(0x00437D40).jmp(0x00437D5C);
}

void SetScoreboardHidden(bool hidden)
{
    g_scoreboard_force_hide = hidden;
}
