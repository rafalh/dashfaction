#include "stdafx.h"
#include "scoreboard.h"
#include "utils.h"
#include "rf.h"
#include "rfproto.h"
#include "kill.h"
#include "spectate_mode.h"
#include "main.h"

using namespace rf;

constexpr float ENTER_ANIM_MS = 100.0f;
constexpr float LEAVE_ANIM_MS = 100.0f;
constexpr float HALF_ENTER_ANIM_MS = ENTER_ANIM_MS / 2.0f;
constexpr float HALF_LEAVE_ANIM_MS = LEAVE_ANIM_MS / 2.0f;

static bool g_ScoreboardForceHide = false;
static bool g_ScoreboardVisible = false;
static unsigned g_AnimTicks = 0;
static bool g_EnterAnim = false;
static bool g_LeaveAnim = false;

auto DrawScoreboardInternal_Hook = makeFunHook(DrawScoreboardInternal);

static int ScoreboardSortFunc(const void *Ptr1, const void *Ptr2)
{
    CPlayer *pPlayer1 = *((CPlayer**)Ptr1), *pPlayer2 = *((CPlayer**)Ptr2);
    return pPlayer2->pStats->iScore - pPlayer1->pStats->iScore;
}

void DrawScoreboardInternal_New(bool bDraw)
{
    if (g_ScoreboardForceHide || !bDraw)
        return;

    unsigned cLeftCol = 0, cRightCol = 0;
    char szBuf[512];
    unsigned GameType = GetGameType();
    
    // Sort players by score
    CPlayer *Players[32];
    CPlayer *pPlayer = *g_ppPlayersList;
    unsigned cPlayers = 0;
    while (cPlayers < 32)
    {
        Players[cPlayers++] = pPlayer;
        if (GameType == RF_DM || !pPlayer->bBlueTeam)
            ++cLeftCol;
        else
            ++cRightCol;
            
        pPlayer = pPlayer->pNext;
        if (!pPlayer || pPlayer == *g_ppPlayersList)
            break;
    }
    qsort(Players, cPlayers, sizeof(CPlayer*), ScoreboardSortFunc);

    // Animation
    float fAnimProgress = 1.0f, fProgressW = 1.0f, fProgressH = 1.0f;
    if (g_gameConfig.scoreboardAnim)
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
    unsigned cx = std::min((GameType == RF_DM) ? 450u : 700u, GrGetViewportWidth());
    unsigned cy = ((GameType == RF_DM) ? 130 : 190) + std::max(cLeftCol, cRightCol) * ROW_H; // DM doesnt show team scores
    cx = (unsigned)(fProgressW * cx);
    cy = (unsigned)(fProgressH * cy);
    unsigned x = (GrGetViewportWidth() - cx) / 2;
    unsigned y = (GrGetViewportHeight() - cy) / 2;
    unsigned xCenter = x + cx / 2;
    GrSetColor(0, 0, 0, 0x80);
    GrDrawRect(x, y, cx, cy, *g_pGrRectMaterial);
    y += 10;

    if (fProgressH < 1.0f || fProgressW < 1.0f)
        return;
    
    // Draw RF logo
    GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    static int ScoreRflogoBm = BmLoad("score_rflogo.tga", -1, TRUE);
    GrDrawImage(ScoreRflogoBm, xCenter - 170, y, *g_pGrImageMaterial);
    y += 30;
    
    // Draw Game Type name
    const char *pszGameTypeName;
    if (GameType == RF_DM)
        pszGameTypeName = g_ppszStringsTable[974];
    else if (GameType == RF_CTF)
        pszGameTypeName = g_ppszStringsTable[975];
    else
        pszGameTypeName = g_ppszStringsTable[976];
    GrDrawAlignedText(GR_ALIGN_CENTER, xCenter, y, pszGameTypeName, *g_pMediumFontId, *g_pGrTextMaterial);
    y += 20;

    // Draw level
    GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
    sprintf(szBuf, "%s (%s) by %s", CString_CStr(g_pstrLevelName), CString_CStr(g_pstrLevelFilename), CString_CStr(g_pstrLevelAuthor));
    CString strLevelInfo, strLevelInfoNew;
    CString_Init(&strLevelInfo, szBuf);
    GrFitText(&strLevelInfoNew, strLevelInfo, cx - 20); // Note: this destroys input string
    GrDrawAlignedText(GR_ALIGN_CENTER, xCenter, y, CString_CStr(&strLevelInfoNew), -1, *g_pGrTextMaterial);
    CString_Destroy(&strLevelInfoNew);
    y += 15;

    // Draw server info
    unsigned i = sprintf(szBuf, "%s (", CString_CStr(g_pstrServName));
    NwAddrToStr(szBuf + i, sizeof(szBuf) - i, g_pServAddr);
    i += strlen(szBuf + i);
    sprintf(szBuf + i, ")");
    CString strServerInfo, strServerInfoNew;
    CString_Init(&strServerInfo, szBuf);
    GrFitText(&strServerInfoNew, strServerInfo, cx - 20); // Note: this destroys input string
    GrDrawAlignedText(GR_ALIGN_CENTER, xCenter, y, CString_CStr(&strServerInfoNew), -1, *g_pGrTextMaterial);
    CString_Destroy(&strServerInfoNew);
    y += 20;
    
    // Draw team scores
    unsigned RedScore = 0, BlueScore = 0;
    if (GameType == RF_CTF)
    {
        static int HudFlagRedBm = BmLoad("hud_flag_red.tga", -1, TRUE);
        static int HudFlagBlueBm = BmLoad("hud_flag_blue.tga", -1, TRUE);
        GrDrawImage(HudFlagRedBm, x + cx * 2 / 6, y, *g_pGrImageMaterial);
        GrDrawImage(HudFlagBlueBm, x + cx * 4 / 6, y, *g_pGrImageMaterial);
        RedScore = CtfGetRedScore();
        BlueScore = CtfGetBlueScore();
    }
    else if (GameType == RF_TEAMDM)
    {
        RedScore = TdmGetRedScore();
        BlueScore = TdmGetBlueScore();
    }
    
    if (GameType != RF_DM)
    {
        GrSetColor(0xD0, 0x20, 0x20, 0xFF);
        sprintf(szBuf, "%u", RedScore);
        GrDrawAlignedText(GR_ALIGN_CENTER, x + cx * 1 / 6, y, szBuf, *g_pBigFontId, *g_pGrTextMaterial);
        GrSetColor(0x20, 0x20, 0xD0, 0xFF);
        sprintf(szBuf, "%u", BlueScore);
        GrDrawAlignedText(GR_ALIGN_CENTER, x + cx * 5 / 6, y, szBuf, *g_pBigFontId, *g_pGrTextMaterial);
        y += 60;
    }
    
    struct
    {
        int StatusBm, Name, Score, KillsDeaths, CtfFlags, Ping;
    } ColOffsets[2];

    // Draw headers
    unsigned NumSect = (GameType == RF_DM ? 1 : 2);
    unsigned cxNameMax = cx / NumSect - 25 - 50 * (GameType == RF_CTF ? 3 : 2) - 70;
    GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    for (unsigned i = 0; i < NumSect; ++i)
    {
        int xCol = x + i * (cx / 2) + 13;
        ColOffsets[i].StatusBm = xCol;
        xCol += 12;

        ColOffsets[i].Name = xCol;
        GrDrawText(xCol, y, g_ppszStringsTable[STR_PLAYER], -1, *g_pGrTextMaterial);
        xCol += cxNameMax;

        ColOffsets[i].Score = xCol;
        GrDrawText(xCol, y, g_ppszStringsTable[STR_SCORE], -1, *g_pGrTextMaterial); // Note: RF uses "Frags"
        xCol += 50;

        ColOffsets[i].KillsDeaths = xCol;
        GrDrawText(xCol, y, "K/D", -1, *g_pGrTextMaterial);
        xCol += 70;

        if (GameType == RF_CTF)
        {
            ColOffsets[i].CtfFlags = xCol;
            GrDrawText(xCol, y, g_ppszStringsTable[STR_CAPS], -1, *g_pGrTextMaterial);
            xCol += 50;
        }

        ColOffsets[i].Ping = xCol;
        GrDrawText(xCol, y, g_ppszStringsTable[STR_PING], -1, *g_pGrTextMaterial);
    }
    y += 20;
    
    CPlayer *pRedFlagPlayer = CtfGetRedFlagPlayer();
    CPlayer *pBlueFlagPlayer = CtfGetBlueFlagPlayer();

    // Finally draw the list
    int SectCounter[2] = { 0, 0 };
    for (unsigned i = 0; i < cPlayers; ++i)
    {
        pPlayer = Players[i];
        
        unsigned SectIdx = GameType == RF_DM || !pPlayer->bBlueTeam ? 0 : 1;
        auto &Offsets = ColOffsets[SectIdx];
        int RowY = y + SectCounter[SectIdx] * ROW_H;
        ++SectCounter[SectIdx];
        
        if (pPlayer == *g_ppPlayersList) // local player
            GrSetColor(0xFF, 0xFF, 0x80, 0xFF);
        else
            GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        
        static int GreenBm = BmLoad("DF_green.tga", -1, 1);
        static int RedBm = BmLoad("DF_red.tga", -1, 1);
        static int HudMicroFlagRedBm = BmLoad("hud_microflag_red.tga", -1, 1);
        static int HudMicroFlagBlueBm = BmLoad("hud_microflag_blue.tga", -1, 1);
        EntityObj *pEntity = HandleToEntity(pPlayer->hEntity);
        int StatusBm = pEntity ? GreenBm : RedBm;
        if (pPlayer == pRedFlagPlayer)
            StatusBm = HudMicroFlagRedBm;
        else if (pPlayer == pBlueFlagPlayer)
            StatusBm = HudMicroFlagBlueBm;
        GrDrawImage(StatusBm, Offsets.StatusBm, RowY + 2, *g_pGrImageMaterial);

        CString strName, strNameNew;
        CString_InitFromStr(&strName, &pPlayer->strName);
        GrFitText(&strNameNew, strName, cxNameMax - 10); // Note: this destroys strName
        GrDrawText(Offsets.Name, RowY, CString_CStr(&strNameNew), -1, *g_pGrTextMaterial);
        CString_Destroy(&strNameNew);
        
        auto pStats = (PlayerStatsNew*)pPlayer->pStats;
        sprintf(szBuf, "%hd", pStats->iScore);
        GrDrawText(Offsets.Score, RowY, szBuf, -1, *g_pGrTextMaterial);

        sprintf(szBuf, "%hd/%hd", pStats->cKills, pStats->cDeaths);
        GrDrawText(Offsets.KillsDeaths, RowY, szBuf, -1, *g_pGrTextMaterial);
        
        if (GameType == RF_CTF)
        {
            sprintf(szBuf, "%hu", pStats->cCaps);
            GrDrawText(Offsets.CtfFlags, RowY, szBuf, -1, *g_pGrTextMaterial);
        }
        
        if (pPlayer->pNwData)
        {
            sprintf(szBuf, "%hu", pPlayer->pNwData->dwPing);
            GrDrawText(Offsets.Ping, RowY, szBuf, -1, *g_pGrTextMaterial);
        }
    }
}

void HudRender_00437BC0()
{
    if (!*g_pbNetworkGame || !*g_ppLocalPlayer)
        return;

    bool bShowScoreboard = (IsEntityCtrlActive(&(*g_ppLocalPlayer)->Settings.Controls, GC_MP_STATS, 0) ||
        (!SpectateModeIsActive() && (IsPlayerEntityInvalid(*g_ppLocalPlayer) || IsPlayerDying(*g_ppLocalPlayer))) ||
        GetCurrentMenuId() == MENU_MP_LIMBO);

    if (g_gameConfig.scoreboardAnim)
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
        DrawScoreboard(true);
}

void InitScoreboard(void)
{
    DrawScoreboardInternal_Hook.hook(DrawScoreboardInternal_New);

    AsmWritter(0x00437BC0)
        .callLong((uintptr_t)&HudRender_00437BC0)
        .jmpLong(0x00437C24);
    AsmWritter(0x00437D40)
        .jmpNear(0x00437D5C);
}

void SetScoreboardHidden(bool hidden)
{
    g_ScoreboardForceHide = hidden;
}
