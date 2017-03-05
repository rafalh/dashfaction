#include "stdafx.h"
#include "scoreboard.h"
#include "utils.h"
#include "rf.h"
#include "rfproto.h"
#include "kill.h"

using namespace rf;

static bool g_ScoreboardHidden = false;

static int ScoreboardSortFunc(const void *Ptr1, const void *Ptr2)
{
    CPlayer *pPlayer1 = *((CPlayer**)Ptr1), *pPlayer2 = *((CPlayer**)Ptr2);
    return pPlayer2->pStats->iScore - pPlayer1->pStats->iScore;
}

void DrawScoreboardInternalHook(BOOL bDraw)
{
    if (g_ScoreboardHidden) return;
    if (!bDraw) return;

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
    
    // Draw background
    constexpr int ROW_H = 15;
    unsigned cx = std::min((GameType == RF_DM) ? 450u : 700u, GrGetViewportWidth());
    unsigned cy = ((GameType == RF_DM) ? 110 : 170) + std::max(cLeftCol, cRightCol) * ROW_H; // DM doesnt show team scores
    unsigned x = (GrGetViewportWidth() - cx) / 2;
    unsigned y = (GrGetViewportHeight() - cy) / 2;
    unsigned xCenter = x + cx / 2;
    GrSetColor(0, 0, 0, 0x80);
    GrDrawRect(x, y, cx, cy, *g_pGrRectMaterial);
    
    // Draw RF logo
    GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    static int ScoreRflogoBm = BmLoad("score_rflogo.tga", -1, TRUE);
    GrDrawImage(ScoreRflogoBm, xCenter - 170, y + 10, *g_pGrImageMaterial);
    
    // Draw level
    GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
    sprintf(szBuf, "%s (%s) by %s", CString_CStr(g_pstrLevelName), CString_CStr(g_pstrLevelFilename), CString_CStr(g_pstrLevelAuthor));
    CString strLevelInfo, strLevelInfoNew;
    CString_Init(&strLevelInfo, szBuf);
    GrFitText(&strLevelInfoNew, strLevelInfo, cx - 20); // Note: this destroys input string
    GrDrawAlignedText(GR_ALIGN_CENTER, xCenter, y + 45, CString_CStr(&strLevelInfoNew), -1, *g_pGrTextMaterial);
    CString_Destroy(&strLevelInfoNew);

    // Draw server info
    unsigned i = sprintf(szBuf, "%s (", CString_CStr(g_pstrServName));
    NwAddrToStr(szBuf + i, sizeof(szBuf) - i, g_pServAddr);
    i += strlen(szBuf + i);
    sprintf(szBuf + i, ")");
    CString strServerInfo, strServerInfoNew;
    CString_Init(&strServerInfo, szBuf);
    GrFitText(&strServerInfoNew, strServerInfo, cx - 20); // Note: this destroys input string
    GrDrawAlignedText(GR_ALIGN_CENTER, xCenter, y + 60, CString_CStr(&strServerInfoNew), -1, *g_pGrTextMaterial);
    y += 80;
    
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

void InitScoreboard(void)
{
    WriteMemUInt8(0x00470880, ASM_LONG_JMP_REL);
    WriteMemInt32(0x00470880 + 1, (uintptr_t)DrawScoreboardInternalHook - (0x00470880 + 0x5));
}

void SetScoreboardHidden(bool hidden)
{
    g_ScoreboardHidden = hidden;
}
