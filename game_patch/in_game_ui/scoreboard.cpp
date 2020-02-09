#include "scoreboard.h"
#include "../multi/kill.h"
#include "../rf/graphics.h"
#include "../rf/network.h"
#include "../rf/misc.h"
#include "../rf/entity.h"
#include "../rf/game_seq.h"
#include "../main.h"
#include <common/rfproto.h>
#include "spectate_mode.h"
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <algorithm>

namespace rf
{
static auto& DrawScoreboard = AddrAsRef<void(bool draw)>(0x00470860);
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

void DrawScoreboardInternal_New(bool draw)
{
    if (g_scoreboard_force_hide || !draw)
        return;

    unsigned c_left_col = 0, c_right_col = 0;
    int game_type = rf::MpGetGameMode();
    bool single_column = game_type == RF_DM;

    // Sort players by score
    rf::Player* players[32];
    unsigned c_players = 0;
    auto player_list = SinglyLinkedList{rf::player_list};
    for (auto& player : player_list) {
        players[c_players++] = &player;
        if (c_players == 32) {
            break;
        }
        if (single_column || player.team == rf::TEAM_RED)
            ++c_left_col;
        else
            ++c_right_col;
    }
    std::sort(players, players + c_players, [](auto player1, auto player2) {
        return player1->stats->score > player2->stats->score;
    });

    // Animation
    float anim_progress = 1.0f, progress_w = 1.0f, progress_h = 1.0f;
    if (g_game_config.scoreboard_anim) {
        unsigned anim_delta = GetTickCount() - g_anim_ticks;
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

    // Draw background
    constexpr int row_h = 15;
    unsigned cx = std::min(single_column ? 450u : 700u, rf::GrGetViewportWidth());
    unsigned num_rows = std::max(c_left_col, c_right_col);
    unsigned cy = (single_column ? 130 : 190) + num_rows * row_h; // DM doesnt show team scores
    cx = static_cast<unsigned>(progress_w * cx);
    cy = static_cast<unsigned>(progress_h * cy);
    unsigned x = (rf::GrGetViewportWidth() - cx) / 2;
    unsigned y = (rf::GrGetViewportHeight() - cy) / 2;
    unsigned x_center = x + cx / 2;
    rf::GrSetColor(0, 0, 0, 0x80);
    rf::GrDrawRect(x, y, cx, cy, rf::gr_rect_material);
    y += 10;

    if (progress_h < 1.0f || progress_w < 1.0f)
        return;

    // Draw RF logo
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    static int score_rflogo_bm = rf::BmLoad("score_rflogo.tga", -1, true);
    rf::GrDrawImage(score_rflogo_bm, x_center - 170, y, rf::gr_image_material);
    y += 30;

    // Draw Game Type name
    const char* game_type_name;
    if (game_type == RF_DM)
        game_type_name = rf::strings::array[974];
    else if (game_type == RF_CTF)
        game_type_name = rf::strings::array[975];
    else
        game_type_name = rf::strings::array[976];
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x_center, y, game_type_name, rf::medium_font_id, rf::gr_text_material);
    y += 20;

    // Draw level
    rf::GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
    auto level_info = rf::String::Format("%s (%s) by %s", rf::level_name.CStr(), rf::level_filename.CStr(),
                                         rf::level_author.CStr());
    rf::String level_info_stripped;
    rf::GrFitText(&level_info_stripped, level_info, cx - 20); // Note: this destroys input string
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x_center, y, level_info_stripped, -1, rf::gr_text_material);
    y += 15;

    // Draw server info
    char ip_addr_buf[64];
    rf::NwAddrToStr(ip_addr_buf, sizeof(ip_addr_buf), rf::serv_addr);
    auto server_info = rf::String::Format("%s (%s)", rf::serv_name.CStr(), ip_addr_buf);
    rf::String server_info_stripped;
    rf::GrFitText(&server_info_stripped, server_info, cx - 20); // Note: this destroys input string
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x_center, y, server_info_stripped, -1, rf::gr_text_material);
    y += 20;

    // Draw team scores
    unsigned red_score = 0, blue_score = 0;
    if (game_type == RF_CTF) {
        static int hud_flag_red_bm = rf::BmLoad("hud_flag_red.tga", -1, true);
        static int hud_flag_blue_bm = rf::BmLoad("hud_flag_blue.tga", -1, true);
        rf::GrDrawImage(hud_flag_red_bm, x + cx * 2 / 6, y, rf::gr_image_material);
        rf::GrDrawImage(hud_flag_blue_bm, x + cx * 4 / 6, y, rf::gr_image_material);
        red_score = rf::CtfGetRedScore();
        blue_score = rf::CtfGetBlueScore();
    }
    else if (game_type == RF_TEAMDM) {
        red_score = rf::TdmGetRedScore();
        blue_score = rf::TdmGetBlueScore();
    }

    if (game_type != RF_DM) {
        rf::GrSetColor(0xD0, 0x20, 0x20, 0xFF);
        auto red_score_str = std::to_string(red_score);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 1 / 6, y, red_score_str.c_str(), rf::big_font_id,
                              rf::gr_text_material);
        rf::GrSetColor(0x20, 0x20, 0xD0, 0xFF);
        auto blue_score_str = std::to_string(blue_score);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 5 / 6, y, blue_score_str.c_str(), rf::big_font_id,
                              rf::gr_text_material);
        y += 60;
    }

    struct
    {
        int status_bm, name, score, kills_deaths, ctf_flags, ping;
    } col_offsets[2];

    // Draw headers
    unsigned num_sect = single_column ? 1 : 2;
    unsigned cx_name_max = cx / num_sect - 25 - 50 * (game_type == RF_CTF ? 3 : 2) - 70;
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    for (unsigned i = 0; i < num_sect; ++i) {
        int x_col = x + i * (cx / 2) + 13;
        col_offsets[i].status_bm = x_col;
        x_col += 12;

        col_offsets[i].name = x_col;
        rf::GrDrawText(x_col, y, rf::strings::player, -1, rf::gr_text_material);
        x_col += cx_name_max;

        col_offsets[i].score = x_col;
        rf::GrDrawText(x_col, y, rf::strings::score, -1, rf::gr_text_material); // Note: RF uses "Frags"
        x_col += 50;

        col_offsets[i].kills_deaths = x_col;
        rf::GrDrawText(x_col, y, "K/D", -1, rf::gr_text_material);
        x_col += 70;

        if (game_type == RF_CTF) {
            col_offsets[i].ctf_flags = x_col;
            rf::GrDrawText(x_col, y, rf::strings::caps, -1, rf::gr_text_material);
            x_col += 50;
        }

        col_offsets[i].ping = x_col;
        rf::GrDrawText(x_col, y, rf::strings::ping, -1, rf::gr_text_material);
    }
    y += 20;

    rf::Player* red_flag_player = rf::CtfGetRedFlagPlayer();
    rf::Player* blue_flag_player = rf::CtfGetBlueFlagPlayer();

    // Finally draw the list
    int sect_counter[2] = {0, 0};
    for (unsigned i = 0; i < c_players; ++i) {
        rf::Player* player = players[i];

        unsigned sect_idx = (single_column || player->team == rf::TEAM_RED) ? 0 : 1;
        auto& offsets = col_offsets[sect_idx];
        int row_y = y + sect_counter[sect_idx] * row_h;
        ++sect_counter[sect_idx];

        if (player == rf::player_list) // local player
            rf::GrSetColor(0xFF, 0xFF, 0x80, 0xFF);
        else
            rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);

        static int green_bm = rf::BmLoad("DF_green.tga", -1, true);
        static int red_bm = rf::BmLoad("DF_red.tga", -1, true);
        static int hud_micro_flag_red_bm = rf::BmLoad("hud_microflag_red.tga", -1, true);
        static int hud_micro_flag_blue_bm = rf::BmLoad("hud_microflag_blue.tga", -1, true);
        rf::EntityObj* entity = rf::EntityGetFromHandle(player->entity_handle);
        int status_bm = entity ? green_bm : red_bm;
        if (player == red_flag_player)
            status_bm = hud_micro_flag_red_bm;
        else if (player == blue_flag_player)
            status_bm = hud_micro_flag_blue_bm;
        rf::GrDrawImage(status_bm, offsets.status_bm, row_y + 2, rf::gr_image_material);

        rf::String player_name_stripped;
        rf::GrFitText(&player_name_stripped, player->name, cx_name_max - 10); // Note: this destroys Name
        // player_name.Forget();
        rf::GrDrawText(offsets.name, row_y, player_name_stripped, -1, rf::gr_text_material);

        auto stats = static_cast<PlayerStatsNew*>(player->stats);
        auto score_str = std::to_string(stats->score);
        rf::GrDrawText(offsets.score, row_y, score_str.c_str(), -1, rf::gr_text_material);

        auto kills_deaths_str = StringFormat("%hd/%hd", stats->num_kills, stats->num_deaths);
        rf::GrDrawText(offsets.kills_deaths, row_y, kills_deaths_str.c_str(), -1, rf::gr_text_material);

        if (game_type == RF_CTF) {
            auto caps_str = std::to_string(stats->caps);
            rf::GrDrawText(offsets.ctf_flags, row_y, caps_str.c_str(), -1, rf::gr_text_material);
        }

        if (player->nw_data) {
            auto ping_str = std::to_string(player->nw_data->ping);
            rf::GrDrawText(offsets.ping, row_y, ping_str.c_str(), -1, rf::gr_text_material);
        }
    }
}

FunHook<void(bool)> DrawScoreboardInternal_hook{0x00470880, DrawScoreboardInternal_New};

void HudRender_00437BC0()
{
    if (!rf::is_net_game || !rf::local_player)
        return;

    bool scoreboard_control_active = rf::IsEntityCtrlActive(&rf::local_player->config.controls, rf::GC_MP_STATS, 0);
    bool is_player_dead = rf::IsPlayerEntityInvalid(rf::local_player) || rf::IsPlayerDying(rf::local_player);
    bool limbo = rf::GameSeqGetState() == rf::GS_MP_LIMBO;
    bool show_scoreboard = scoreboard_control_active || (!SpectateModeIsActive() && is_player_dead) || limbo;

    if (g_game_config.scoreboard_anim) {
        if (!g_scoreboard_visible && show_scoreboard) {
            g_enter_anim = true;
            g_leave_anim = false;
            g_anim_ticks = GetTickCount();
            g_scoreboard_visible = true;
        }
        if (g_scoreboard_visible && !show_scoreboard && !g_leave_anim) {
            g_enter_anim = false;
            g_leave_anim = true;
            g_anim_ticks = GetTickCount();
        }
    }
    else
        g_scoreboard_visible = show_scoreboard;

    if (g_scoreboard_visible)
        rf::DrawScoreboard(true);
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
