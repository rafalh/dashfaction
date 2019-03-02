#include "stdafx.h"
#include "scoreboard.h"
#include "utils.h"
#include "rf.h"
#include "rfproto.h"
#include "kill.h"
#include "spectate_mode.h"
#include "main.h"
#include <FunHook2.h>

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
    rf::Player *pPlayer1 = *((rf::Player**)Ptr1), *pPlayer2 = *((rf::Player**)Ptr2);
    return pPlayer2->pStats->iScore - pPlayer1->pStats->iScore;
}

void DrawScoreboardInternal_New(bool bDraw)
{
    if (g_ScoreboardForceHide || !bDraw)
        return;

    unsigned cLeftCol = 0, cRightCol = 0;
    char szBuf[512];
    unsigned GameType = rf::GetGameType();
    
    // Sort players by score
    rf::Player *Players[32];
    rf::Player *pPlayer = rf::g_pPlayersList;
    unsigned cPlayers = 0;
    while (cPlayers < 32)
    {
        Players[cPlayers++] = pPlayer;
        if (GameType == RF_DM || !pPlayer->bBlueTeam)
            ++cLeftCol;
        else
            ++cRightCol;
            
        pPlayer = pPlayer->pNext;
        if (!pPlayer || pPlayer == rf::g_pPlayersList)
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
    static int ScoreRflogoBm = rf::BmLoad("score_rflogo.tga", -1, TRUE);
    rf::GrDrawImage(ScoreRflogoBm, xCenter - 170, y, rf::g_GrImageMaterial);
    y += 30;
    
    // Draw Game Type name
    const char *pszGameTypeName;
    if (GameType == RF_DM)
        pszGameTypeName = rf::g_ppszStringsTable[974];
    else if (GameType == RF_CTF)
        pszGameTypeName = rf::g_ppszStringsTable[975];
    else
        pszGameTypeName = rf::g_ppszStringsTable[976];
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, xCenter, y, pszGameTypeName, rf::g_MediumFontId, rf::g_GrTextMaterial);
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
    rf::NwAddrToStr(ip_addr_buf, sizeof(ip_addr_buf), &rf::g_ServAddr);
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
        sprintf(szBuf, "%u", RedScore);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 1 / 6, y, szBuf, rf::g_BigFontId, rf::g_GrTextMaterial);
        rf::GrSetColor(0x20, 0x20, 0xD0, 0xFF);
        sprintf(szBuf, "%u", BlueScore);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx * 5 / 6, y, szBuf, rf::g_BigFontId, rf::g_GrTextMaterial);
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
        rf::GrDrawText(xCol, y, rf::g_ppszStringsTable[rf::STR_PLAYER], -1, rf::g_GrTextMaterial);
        xCol += cxNameMax;

        ColOffsets[i].Score = xCol;
        rf::GrDrawText(xCol, y, rf::g_ppszStringsTable[rf::STR_SCORE], -1, rf::g_GrTextMaterial); // Note: RF uses "Frags"
        xCol += 50;

        ColOffsets[i].KillsDeaths = xCol;
        rf::GrDrawText(xCol, y, "K/D", -1, rf::g_GrTextMaterial);
        xCol += 70;

        if (GameType == RF_CTF)
        {
            ColOffsets[i].CtfFlags = xCol;
            rf::GrDrawText(xCol, y, rf::g_ppszStringsTable[rf::STR_CAPS], -1, rf::g_GrTextMaterial);
            xCol += 50;
        }

        ColOffsets[i].Ping = xCol;
        rf::GrDrawText(xCol, y, rf::g_ppszStringsTable[rf::STR_PING], -1, rf::g_GrTextMaterial);
    }
    y += 20;
    
    rf::Player *pRedFlagPlayer = rf::CtfGetRedFlagPlayer();
    rf::Player *pBlueFlagPlayer = rf::CtfGetBlueFlagPlayer();

    // Finally draw the list
    int SectCounter[2] = { 0, 0 };
    for (unsigned i = 0; i < cPlayers; ++i)
    {
        pPlayer = Players[i];
        
        unsigned SectIdx = GameType == RF_DM || !pPlayer->bBlueTeam ? 0 : 1;
        auto &Offsets = ColOffsets[SectIdx];
        int RowY = y + SectCounter[SectIdx] * ROW_H;
        ++SectCounter[SectIdx];
        
        if (pPlayer == rf::g_pPlayersList) // local player
            rf::GrSetColor(0xFF, 0xFF, 0x80, 0xFF);
        else
            rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        
        static int GreenBm = rf::BmLoad("DF_green.tga", -1, 1);
        static int RedBm = rf::BmLoad("DF_red.tga", -1, 1);
        static int HudMicroFlagRedBm = rf::BmLoad("hud_microflag_red.tga", -1, 1);
        static int HudMicroFlagBlueBm = rf::BmLoad("hud_microflag_blue.tga", -1, 1);
        rf::EntityObj *pEntity = rf::EntityGetFromHandle(pPlayer->hEntity);
        int StatusBm = pEntity ? GreenBm : RedBm;
        if (pPlayer == pRedFlagPlayer)
            StatusBm = HudMicroFlagRedBm;
        else if (pPlayer == pBlueFlagPlayer)
            StatusBm = HudMicroFlagBlueBm;
        rf::GrDrawImage(StatusBm, Offsets.StatusBm, RowY + 2, rf::g_GrImageMaterial);

        rf::String player_name_stripped;
        rf::GrFitText(&player_name_stripped, pPlayer->strName, cxNameMax - 10); // Note: this destroys strName
        //player_name.Forget();
        rf::GrDrawText(Offsets.Name, RowY, player_name_stripped, -1, rf::g_GrTextMaterial);
        
        auto pStats = (PlayerStatsNew*)pPlayer->pStats;
        sprintf(szBuf, "%hd", pStats->iScore);
        rf::GrDrawText(Offsets.Score, RowY, szBuf, -1, rf::g_GrTextMaterial);

        sprintf(szBuf, "%hd/%hd", pStats->cKills, pStats->cDeaths);
        rf::GrDrawText(Offsets.KillsDeaths, RowY, szBuf, -1, rf::g_GrTextMaterial);
        
        if (GameType == RF_CTF)
        {
            sprintf(szBuf, "%hu", pStats->cCaps);
            rf::GrDrawText(Offsets.CtfFlags, RowY, szBuf, -1, rf::g_GrTextMaterial);
        }
        
        if (pPlayer->pNwData)
        {
            sprintf(szBuf, "%hu", pPlayer->pNwData->dwPing);
            rf::GrDrawText(Offsets.Ping, RowY, szBuf, -1, rf::g_GrTextMaterial);
        }
    }
}

FunHook2<void(bool)> DrawScoreboardInternal_Hook{0x00470880, DrawScoreboardInternal_New};

void HudRender_00437BC0()
{
    if (!rf::g_bNetworkGame || !rf::g_pLocalPlayer)
        return;

    bool bShowScoreboard = (rf::IsEntityCtrlActive(&rf::g_pLocalPlayer->Config.Controls, rf::GC_MP_STATS, 0) ||
        (!SpectateModeIsActive() && (rf::IsPlayerEntityInvalid(rf::g_pLocalPlayer) || rf::IsPlayerDying(rf::g_pLocalPlayer))) ||
        rf::GameSeqGetState() == rf::GS_MP_LIMBO);

    if (g_game_config.scoreboardAnim)
    {
        if (!g_ScoreboardVisible && bShowScoreboard)
        {
            g_EnterAnim = true;
            g_LeaveAnim = false;
            g_AnimTicks = GetTickCount();
            g_ScoreboardVisible = true;
        }
        if (g_ScoreboardVisible && !bShowScoreboard && !g_LeaveAnim)
        {
            g_EnterAnim = false;
            g_LeaveAnim = true;
            g_AnimTicks = GetTickCount();
        }
    }
    else
        g_ScoreboardVisible = bShowScoreboard;

    if (g_ScoreboardVisible)
        rf::DrawScoreboard(true);
}

void InitScoreboard(void)
{
    DrawScoreboardInternal_Hook.Install();

    AsmWritter(0x00437BC0)
        .callLong(HudRender_00437BC0)
        .jmpLong(0x00437C24);
    AsmWritter(0x00437D40)
        .jmpNear(0x00437D5C);
}

void SetScoreboardHidden(bool hidden)
{
    g_ScoreboardForceHide = hidden;
}
