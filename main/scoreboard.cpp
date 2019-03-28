#include "stdafx.h"
#include "scoreboard.h"
#include "utils.h"
#include "rf.h"
#include "rfproto.h"
#include "kill.h"
#include "spectate_mode.h"
#include "main.h"
#include <FunHook2.h>

namespace rf {
    static const auto DrawScoreboard = (void(*)(bool Draw))0x00470860;
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

static int ScoreboardSortFunc(const void *Ptr1, const void *Ptr2)
{
    rf::Player *Player1 = *((rf::Player**)Ptr1), *Player2 = *((rf::Player**)Ptr2);
    return Player2->Stats->score - Player1->Stats->score;
}

void DrawScoreboardInternal_New(bool Draw)
{
    if (g_ScoreboardForceHide || !Draw)
        return;

    unsigned cLeftCol = 0, cRightCol = 0;
    char Buf[512];
    unsigned GameType = rf::GetGameType();

    // Sort players by score
    rf::Player *Players[32];
    rf::Player *Player = rf::g_PlayersList;
    unsigned cPlayers = 0;
    while (cPlayers < 32)
    {
        Players[cPlayers++] = Player;
        if (GameType == RF_DM || !Player->BlueTeam)
            ++cLeftCol;
        else
            ++cRightCol;

        Player = Player->Next;
        if (!Player || Player == rf::g_PlayersList)
            break;
    }
    qsort(Players, cPlayers, sizeof(rf::Player*), ScoreboardSortFunc);

    // Animation
    float fAnimProgress = 1.0f, fProgressW = 1.0f, fProgressH = 1.0f;
    if (g_game_config.scoreboardAnim)
    {
        unsigned AnimDelta = GetTickCount() - g_AnimTicks;
        if (g_EnterAnim)
            fAnimProgress = AnimDelta / ENTER_ANIM_MS;
        else if (g_LeaveAnim)
            fAnimProgress = (LEAVE_ANIM_MS - AnimDelta) / LEAVE_ANIM_MS;

        if (g_LeaveAnim && fAnimProgress <= 0.0f)
        {
            g_ScoreboardVisible = false;
            return;
        }

        fProgressW = fAnimProgress * 2.0f;
        fProgressH = (fAnimProgress - 0.5f) * 2.0f;

        fProgressW = std::min(std::max(fProgressW, 0.1f), 1.0f);
        fProgressH = std::min(std::max(fProgressH, 0.1f), 1.0f);
    }

    // Draw background
    constexpr int ROW_H = 15;
    unsigned cx = std::min((GameType == RF_DM) ? 450u : 700u, rf::GrGetViewportWidth());
    unsigned cy = ((GameType == RF_DM) ? 130 : 190) + std::max(cLeftCol, cRightCol) * ROW_H; // DM doesnt show team scores
    cx = (unsigned)(fProgressW * cx);
    cy = (unsigned)(fProgressH * cy);
    unsigned x = (rf::GrGetViewportWidth() - cx) / 2;
    unsigned y = (rf::GrGetViewportHeight() - cy) / 2;
    unsigned xCenter = x + cx / 2;
    rf::GrSetColor(0, 0, 0, 0x80);
    rf::GrDrawRect(x, y, cx, cy, rf::g_GrRectMaterial);
    y += 10;

    if (fProgressH < 1.0f || fProgressW < 1.0f)
        return;

    // Draw RF logo
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    static int ScoreRflogoBm = rf::BmLoad("score_rflogo.tga", -1, true);
    rf::GrDrawImage(ScoreRflogoBm, xCenter - 170, y, rf::g_GrImageMaterial);
    y += 30;

    // Draw Game Type name
    const char *GameTypeName;
    if (GameType == RF_DM)
        GameTypeName = rf::strings::array[974];
    else if (GameType == RF_CTF)
        GameTypeName = rf::strings::array[975];
    else
        GameTypeName = rf::strings::array[976];
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, xCenter, y, GameTypeName, rf::g_MediumFontId, rf::g_GrTextMaterial);
    y += 20;

    // Draw level
    rf::GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
    auto level_info = rf::String::Format("%s (%s) by %s", rf::g_strLevelName.CStr(), rf::g_strLevelFilename.CStr(),
        rf::g_strLevelAuthor.CStr());
    rf::String level_info_stripped;
    rf::GrFitText(&level_info_stripped, level_info, cx - 20); // Note: this destroys input string
    //level_info.Forget();
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, xCenter, y, level_info_stripped, -1, rf::g_GrTextMaterial);
    y += 15;

    // Draw server info
    char ip_addr_buf[64];
    rf::NwAddrToStr(ip_addr_buf, sizeof(ip_addr_buf), rf::g_ServAddr);
    auto server_info = rf::String::Format("%s (%s)", rf::g_strServName.CStr(), ip_addr_buf);
    rf::String server_info_stripped;
    rf::GrFitText(&server_info_stripped, server_info, cx - 20); // Note: this destroys input string
    //server_info.Forget();
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, xCenter, y, server_info_stripped, -1, rf::g_GrTextMaterial);
    y += 20;

    // Draw team scores
    unsigned RedScore = 0, BlueScore = 0;
    if (GameType == RF_CTF)
    {
        static int HudFlagRedBm = rf::BmLoad("hud_flag_red.tga", -1, true);
        static int HudFlagBlueBm = rf::BmLoad("hud_flag_blue.tga", -1, true);
        rf::GrDrawImage(HudFlagRedBm, x + cx * 2 / 6, y, rf::g_GrImageMaterial);
        rf::GrDrawImage(HudFlagBlueBm, x + cx * 4 / 6, y, rf::g_GrImageMaterial);
        RedScore = rf::CtfGetRedScore();
        BlueScore = rf::CtfGetBlueScore();
    }
    else if (GameType == RF_TEAMDM)
    {
        RedScore = rf::TdmGetRedScore();
        BlueScore = rf::TdmGetBlueScore();
    }

    if (GameType != RF_DM)
    {
        rf::GrSetColor(0xD0, 0x20, 0x20, 0xFF);
        sprintf(Buf, "%u", RedScore);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 1 / 6, y, Buf, rf::g_BigFontId, rf::g_GrTextMaterial);
        rf::GrSetColor(0x20, 0x20, 0xD0, 0xFF);
        sprintf(Buf, "%u", BlueScore);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 5 / 6, y, Buf, rf::g_BigFontId, rf::g_GrTextMaterial);
        y += 60;
    }

    struct
    {
        int StatusBm, Name, Score, KillsDeaths, CtfFlags, Ping;
    } ColOffsets[2];

    // Draw headers
    unsigned NumSect = (GameType == RF_DM ? 1 : 2);
    unsigned cxNameMax = cx / NumSect - 25 - 50 * (GameType == RF_CTF ? 3 : 2) - 70;
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    for (unsigned i = 0; i < NumSect; ++i)
    {
        int xCol = x + i * (cx / 2) + 13;
        ColOffsets[i].StatusBm = xCol;
        xCol += 12;

        ColOffsets[i].Name = xCol;
        rf::GrDrawText(xCol, y, rf::strings::player, -1, rf::g_GrTextMaterial);
        xCol += cxNameMax;

        ColOffsets[i].Score = xCol;
        rf::GrDrawText(xCol, y, rf::strings::score, -1, rf::g_GrTextMaterial); // Note: RF uses "Frags"
        xCol += 50;

        ColOffsets[i].KillsDeaths = xCol;
        rf::GrDrawText(xCol, y, "K/D", -1, rf::g_GrTextMaterial);
        xCol += 70;

        if (GameType == RF_CTF)
        {
            ColOffsets[i].CtfFlags = xCol;
            rf::GrDrawText(xCol, y, rf::strings::caps, -1, rf::g_GrTextMaterial);
            xCol += 50;
        }

        ColOffsets[i].Ping = xCol;
        rf::GrDrawText(xCol, y, rf::strings::ping, -1, rf::g_GrTextMaterial);
    }
    y += 20;

    rf::Player *RedFlagPlayer = rf::CtfGetRedFlagPlayer();
    rf::Player *BlueFlagPlayer = rf::CtfGetBlueFlagPlayer();

    // Finally draw the list
    int SectCounter[2] = { 0, 0 };
    for (unsigned i = 0; i < cPlayers; ++i)
    {
        Player = Players[i];

        unsigned SectIdx = GameType == RF_DM || !Player->BlueTeam ? 0 : 1;
        auto &Offsets = ColOffsets[SectIdx];
        int RowY = y + SectCounter[SectIdx] * ROW_H;
        ++SectCounter[SectIdx];

        if (Player == rf::g_PlayersList) // local player
            rf::GrSetColor(0xFF, 0xFF, 0x80, 0xFF);
        else
            rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);

        static int GreenBm = rf::BmLoad("DF_green.tga", -1, true);
        static int RedBm = rf::BmLoad("DF_red.tga", -1, true);
        static int HudMicroFlagRedBm = rf::BmLoad("hud_microflag_red.tga", -1, true);
        static int HudMicroFlagBlueBm = rf::BmLoad("hud_microflag_blue.tga", -1, true);
        rf::EntityObj *Entity = rf::EntityGetFromHandle(Player->hEntity);
        int StatusBm = Entity ? GreenBm : RedBm;
        if (Player == RedFlagPlayer)
            StatusBm = HudMicroFlagRedBm;
        else if (Player == BlueFlagPlayer)
            StatusBm = HudMicroFlagBlueBm;
        rf::GrDrawImage(StatusBm, Offsets.StatusBm, RowY + 2, rf::g_GrImageMaterial);

        rf::String player_name_stripped;
        rf::GrFitText(&player_name_stripped, Player->strName, cxNameMax - 10); // Note: this destroys strName
        //player_name.Forget();
        rf::GrDrawText(Offsets.Name, RowY, player_name_stripped, -1, rf::g_GrTextMaterial);

        auto Stats = (PlayerStatsNew*)Player->Stats;
        sprintf(Buf, "%hd", Stats->score);
        rf::GrDrawText(Offsets.Score, RowY, Buf, -1, rf::g_GrTextMaterial);

        sprintf(Buf, "%hd/%hd", Stats->num_kills, Stats->num_deaths);
        rf::GrDrawText(Offsets.KillsDeaths, RowY, Buf, -1, rf::g_GrTextMaterial);

        if (GameType == RF_CTF)
        {
            sprintf(Buf, "%hu", Stats->caps);
            rf::GrDrawText(Offsets.CtfFlags, RowY, Buf, -1, rf::g_GrTextMaterial);
        }

        if (Player->NwData)
        {
            sprintf(Buf, "%hu", Player->NwData->dwPing);
            rf::GrDrawText(Offsets.Ping, RowY, Buf, -1, rf::g_GrTextMaterial);
        }
    }
}

FunHook2<void(bool)> DrawScoreboardInternal_Hook{0x00470880, DrawScoreboardInternal_New};

void HudRender_00437BC0()
{
    if (!rf::g_IsNetworkGame || !rf::g_LocalPlayer)
        return;

    bool ShowScoreboard = (rf::IsEntityCtrlActive(&rf::g_LocalPlayer->Config.Controls, rf::GC_MP_STATS, 0) ||
        (!SpectateModeIsActive() && (rf::IsPlayerEntityInvalid(rf::g_LocalPlayer) || rf::IsPlayerDying(rf::g_LocalPlayer))) ||
        rf::GameSeqGetState() == rf::GS_MP_LIMBO);

    if (g_game_config.scoreboardAnim)
    {
        if (!g_ScoreboardVisible && ShowScoreboard)
        {
            g_EnterAnim = true;
            g_LeaveAnim = false;
            g_AnimTicks = GetTickCount();
            g_ScoreboardVisible = true;
        }
        if (g_ScoreboardVisible && !ShowScoreboard && !g_LeaveAnim)
        {
            g_EnterAnim = false;
            g_LeaveAnim = true;
            g_AnimTicks = GetTickCount();
        }
    }
    else
        g_ScoreboardVisible = ShowScoreboard;

    if (g_ScoreboardVisible)
        rf::DrawScoreboard(true);
}

void InitScoreboard(void)
{
    DrawScoreboardInternal_Hook.Install();

    AsmWritter(0x00437BC0)
        .call(HudRender_00437BC0)
        .jmp(0x00437C24);
    AsmWritter(0x00437D40)
        .jmp(0x00437D5C);
}

void SetScoreboardHidden(bool hidden)
{
    g_ScoreboardForceHide = hidden;
}
