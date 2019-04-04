#include "scoreboard.h"
#include "kill.h"
#include "main.h"
#include "rf.h"
#include "rfproto.h"
#include "spectate_mode.h"
#include "stdafx.h"
#include "utils.h"
#include <FunHook2.h>
#include <algorithm>

namespace rf
{
static const auto DrawScoreboard = (void (*)(bool draw))0x00470860;
}

constexpr float ENTER_ANIM_MS = 100.0f;
constexpr float LEAVE_ANIM_MS = 100.0f;
constexpr float HALF_ENTER_ANIM_MS = ENTER_ANIM_MS / 2.0f;
constexpr float HALF_LEAVE_ANIM_MS = LEAVE_ANIM_MS / 2.0f;

static bool g_ScoreboardForceHide = false;
static bool g_ScoreboardVisible = false;
static unsigned g_AnimTicks = 0;
static bool g_EnterAnim = false;
static bool g_LeaveAnim = false;

static int ScoreboardSortFunc(const void* ptr1, const void* ptr2)
{
    rf::Player* player1 = *((rf::Player**)ptr1);
    rf::Player* player2 = *((rf::Player**)ptr2);
    return player2->Stats->score - player1->Stats->score;
}

void DrawScoreboardInternal_New(bool draw)
{
    if (g_ScoreboardForceHide || !draw)
        return;

    unsigned c_left_col = 0, c_right_col = 0;
    unsigned game_type = rf::GetGameType();

    // Sort players by score
    rf::Player* players[32];
    rf::Player* player = rf::g_PlayersList;
    unsigned c_players = 0;
    while (c_players < 32) {
        players[c_players++] = player;
        if (game_type == RF_DM || !player->BlueTeam)
            ++c_left_col;
        else
            ++c_right_col;

        player = player->Next;
        if (!player || player == rf::g_PlayersList)
            break;
    }
    qsort(players, c_players, sizeof(rf::Player*), ScoreboardSortFunc);

    // Animation
    float f_anim_progress = 1.0f, f_progress_w = 1.0f, f_progress_h = 1.0f;
    if (g_game_config.scoreboardAnim) {
        unsigned anim_delta = GetTickCount() - g_AnimTicks;
        if (g_EnterAnim)
            f_anim_progress = anim_delta / ENTER_ANIM_MS;
        else if (g_LeaveAnim)
            f_anim_progress = (LEAVE_ANIM_MS - anim_delta) / LEAVE_ANIM_MS;

        if (g_LeaveAnim && f_anim_progress <= 0.0f) {
            g_ScoreboardVisible = false;
            return;
        }

        f_progress_w = f_anim_progress * 2.0f;
        f_progress_h = (f_anim_progress - 0.5f) * 2.0f;

        f_progress_w = std::clamp(f_progress_w, 0.1f, 1.0f);
        f_progress_h = std::clamp(f_progress_h, 0.1f, 1.0f);
    }

    // Draw background
    constexpr int row_h = 15;
    unsigned cx = std::min((game_type == RF_DM) ? 450u : 700u, rf::GrGetViewportWidth());
    unsigned cy =
        ((game_type == RF_DM) ? 130 : 190) + std::max(c_left_col, c_right_col) * row_h; // DM doesnt show team scores
    cx = (unsigned)(f_progress_w * cx);
    cy = (unsigned)(f_progress_h * cy);
    unsigned x = (rf::GrGetViewportWidth() - cx) / 2;
    unsigned y = (rf::GrGetViewportHeight() - cy) / 2;
    unsigned x_center = x + cx / 2;
    rf::GrSetColor(0, 0, 0, 0x80);
    rf::GrDrawRect(x, y, cx, cy, rf::g_GrRectMaterial);
    y += 10;

    if (f_progress_h < 1.0f || f_progress_w < 1.0f)
        return;

    // Draw RF logo
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    static int score_rflogo_bm = rf::BmLoad("score_rflogo.tga", -1, true);
    rf::GrDrawImage(score_rflogo_bm, x_center - 170, y, rf::g_GrImageMaterial);
    y += 30;

    // Draw Game Type name
    const char* game_type_name;
    if (game_type == RF_DM)
        game_type_name = rf::strings::array[974];
    else if (game_type == RF_CTF)
        game_type_name = rf::strings::array[975];
    else
        game_type_name = rf::strings::array[976];
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x_center, y, game_type_name, rf::g_MediumFontId, rf::g_GrTextMaterial);
    y += 20;

    // Draw level
    rf::GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
    auto level_info = rf::String::Format("%s (%s) by %s", rf::g_strLevelName.CStr(), rf::g_strLevelFilename.CStr(),
                                         rf::g_strLevelAuthor.CStr());
    rf::String level_info_stripped;
    rf::GrFitText(&level_info_stripped, level_info, cx - 20); // Note: this destroys input string
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x_center, y, level_info_stripped, -1, rf::g_GrTextMaterial);
    y += 15;

    // Draw server info
    char ip_addr_buf[64];
    rf::NwAddrToStr(ip_addr_buf, sizeof(ip_addr_buf), rf::g_ServAddr);
    auto server_info = rf::String::Format("%s (%s)", rf::g_strServName.CStr(), ip_addr_buf);
    rf::String server_info_stripped;
    rf::GrFitText(&server_info_stripped, server_info, cx - 20); // Note: this destroys input string
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x_center, y, server_info_stripped, -1, rf::g_GrTextMaterial);
    y += 20;

    // Draw team scores
    unsigned red_score = 0, blue_score = 0;
    if (game_type == RF_CTF) {
        static int hud_flag_red_bm = rf::BmLoad("hud_flag_red.tga", -1, true);
        static int hud_flag_blue_bm = rf::BmLoad("hud_flag_blue.tga", -1, true);
        rf::GrDrawImage(hud_flag_red_bm, x + cx * 2 / 6, y, rf::g_GrImageMaterial);
        rf::GrDrawImage(hud_flag_blue_bm, x + cx * 4 / 6, y, rf::g_GrImageMaterial);
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
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 1 / 6, y, red_score_str.c_str(), rf::g_BigFontId,
                              rf::g_GrTextMaterial);
        rf::GrSetColor(0x20, 0x20, 0xD0, 0xFF);
        auto blue_score_str = std::to_string(blue_score);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 5 / 6, y, blue_score_str.c_str(), rf::g_BigFontId,
                              rf::g_GrTextMaterial);
        y += 60;
    }

    struct
    {
        int StatusBm, Name, Score, KillsDeaths, CtfFlags, Ping;
    } col_offsets[2];

    // Draw headers
    unsigned num_sect = (game_type == RF_DM ? 1 : 2);
    unsigned cx_name_max = cx / num_sect - 25 - 50 * (game_type == RF_CTF ? 3 : 2) - 70;
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    for (unsigned i = 0; i < num_sect; ++i) {
        int x_col = x + i * (cx / 2) + 13;
        col_offsets[i].StatusBm = x_col;
        x_col += 12;

        col_offsets[i].Name = x_col;
        rf::GrDrawText(x_col, y, rf::strings::player, -1, rf::g_GrTextMaterial);
        x_col += cx_name_max;

        col_offsets[i].Score = x_col;
        rf::GrDrawText(x_col, y, rf::strings::score, -1, rf::g_GrTextMaterial); // Note: RF uses "Frags"
        x_col += 50;

        col_offsets[i].KillsDeaths = x_col;
        rf::GrDrawText(x_col, y, "K/D", -1, rf::g_GrTextMaterial);
        x_col += 70;

        if (game_type == RF_CTF) {
            col_offsets[i].CtfFlags = x_col;
            rf::GrDrawText(x_col, y, rf::strings::caps, -1, rf::g_GrTextMaterial);
            x_col += 50;
        }

        col_offsets[i].Ping = x_col;
        rf::GrDrawText(x_col, y, rf::strings::ping, -1, rf::g_GrTextMaterial);
    }
    y += 20;

    rf::Player* red_flag_player = rf::CtfGetRedFlagPlayer();
    rf::Player* blue_flag_player = rf::CtfGetBlueFlagPlayer();

    // Finally draw the list
    int sect_counter[2] = {0, 0};
    for (unsigned i = 0; i < c_players; ++i) {
        player = players[i];

        unsigned sect_idx = game_type == RF_DM || !player->BlueTeam ? 0 : 1;
        auto& offsets = col_offsets[sect_idx];
        int row_y = y + sect_counter[sect_idx] * row_h;
        ++sect_counter[sect_idx];

        if (player == rf::g_PlayersList) // local player
            rf::GrSetColor(0xFF, 0xFF, 0x80, 0xFF);
        else
            rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);

        static int green_bm = rf::BmLoad("DF_green.tga", -1, true);
        static int red_bm = rf::BmLoad("DF_red.tga", -1, true);
        static int hud_micro_flag_red_bm = rf::BmLoad("hud_microflag_red.tga", -1, true);
        static int hud_micro_flag_blue_bm = rf::BmLoad("hud_microflag_blue.tga", -1, true);
        rf::EntityObj* entity = rf::EntityGetFromHandle(player->Entity_handle);
        int status_bm = entity ? green_bm : red_bm;
        if (player == red_flag_player)
            status_bm = hud_micro_flag_red_bm;
        else if (player == blue_flag_player)
            status_bm = hud_micro_flag_blue_bm;
        rf::GrDrawImage(status_bm, offsets.StatusBm, row_y + 2, rf::g_GrImageMaterial);

        rf::String player_name_stripped;
        rf::GrFitText(&player_name_stripped, player->strName, cx_name_max - 10); // Note: this destroys strName
        // player_name.Forget();
        rf::GrDrawText(offsets.Name, row_y, player_name_stripped, -1, rf::g_GrTextMaterial);

        auto stats = (PlayerStatsNew*)player->Stats;
        auto score_str = std::to_string(stats->score);
        rf::GrDrawText(offsets.Score, row_y, score_str.c_str(), -1, rf::g_GrTextMaterial);

        auto kills_deaths_str = StringFormat("%hd/%hd", stats->num_kills, stats->num_deaths);
        rf::GrDrawText(offsets.KillsDeaths, row_y, kills_deaths_str.c_str(), -1, rf::g_GrTextMaterial);

        if (game_type == RF_CTF) {
            auto caps_str = std::to_string(stats->caps);
            rf::GrDrawText(offsets.CtfFlags, row_y, caps_str.c_str(), -1, rf::g_GrTextMaterial);
        }

        if (player->NwData) {
            auto ping_str = std::to_string(player->NwData->dwPing);
            rf::GrDrawText(offsets.Ping, row_y, ping_str.c_str(), -1, rf::g_GrTextMaterial);
        }
    }
}

FunHook2<void(bool)> DrawScoreboardInternal_Hook{0x00470880, DrawScoreboardInternal_New};

void HudRender_00437BC0()
{
    if (!rf::g_IsNetworkGame || !rf::g_LocalPlayer)
        return;

    bool scoreboard_control_active = rf::IsEntityCtrlActive(&rf::g_LocalPlayer->Config.Controls, rf::GC_MP_STATS, 0);
    bool is_player_dead = rf::IsPlayerEntityInvalid(rf::g_LocalPlayer) || rf::IsPlayerDying(rf::g_LocalPlayer);
    bool limbo = rf::GameSeqGetState() == rf::GS_MP_LIMBO;
    bool show_scoreboard = scoreboard_control_active || (!SpectateModeIsActive() && is_player_dead) || limbo;

    if (g_game_config.scoreboardAnim) {
        if (!g_ScoreboardVisible && show_scoreboard) {
            g_EnterAnim = true;
            g_LeaveAnim = false;
            g_AnimTicks = GetTickCount();
            g_ScoreboardVisible = true;
        }
        if (g_ScoreboardVisible && !show_scoreboard && !g_LeaveAnim) {
            g_EnterAnim = false;
            g_LeaveAnim = true;
            g_AnimTicks = GetTickCount();
        }
    }
    else
        g_ScoreboardVisible = show_scoreboard;

    if (g_ScoreboardVisible)
        rf::DrawScoreboard(true);
}

void InitScoreboard(void)
{
    DrawScoreboardInternal_Hook.Install();

    AsmWritter(0x00437BC0).call(HudRender_00437BC0).jmp(0x00437C24);
    AsmWritter(0x00437D40).jmp(0x00437D5C);
}

void SetScoreboardHidden(bool hidden)
{
    g_ScoreboardForceHide = hidden;
}
