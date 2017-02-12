#include "stdafx.h"
#include "scoreboard.h"
#include "utils.h"
#include "rf.h"
#include "rfproto.h"

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
    unsigned cx = (GameType == RF_DM) ? 450 : 600;
    unsigned cy = ((GameType == RF_DM) ? 110 : 170) + std::max(cLeftCol, cRightCol) * ROW_H; // DM doesnt show team scores
    unsigned x = (GrGetViewportWidth() - cx) / 2;
    unsigned y = (GrGetViewportHeight() - cy) / 2;
    GrSetColor(0, 0, 0, 0x80);
    GrDrawRect(x, y, cx, cy, *g_pGrRectMaterial);
    
    // Draw RF logo
    GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    static int ScoreRflogoBm = BmLoad("score_rflogo.tga", -1, TRUE);
    GrDrawImage(ScoreRflogoBm, x + cx / 2 - 170, y + 10, *g_pGrImageMaterial);
    
    // Draw map and server name
    GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
    sprintf(szBuf, "%s (%s) by %s", g_pstrLevelName->psz, g_pstrLevelFilename->psz, g_pstrLevelAuthor->psz);
    GrDrawAlignedText(GR_ALIGN_CENTER, x + cx/2, y + 45, szBuf, -1, *g_pGrTextMaterial);
    unsigned i = sprintf(szBuf, "%s (", g_pstrServName->psz);
    NwAddrToStr(szBuf + i, sizeof(szBuf) - i, g_pServAddr);
    i += strlen(szBuf + i);
    sprintf(szBuf + i, ")");
    GrDrawAlignedText(GR_ALIGN_CENTER, x + cx/2, y + 60, szBuf, -1, *g_pGrTextMaterial);
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
        int StatusBm, Name, Score, CtfFlags, Ping;
    } ColOffsets[2];

    // Draw headers
    unsigned NumColumns = (GameType == RF_DM ? 1 : 2);
    unsigned cxNameMax = cx / NumColumns - 25 - 50 * (GameType == RF_CTF ? 3 : 2);
    GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    for (unsigned i = 0; i < NumColumns; ++i)
    {
        int ColX = x + i * 300 + 13;
        ColOffsets[i].StatusBm = ColX;
        ColX += 12;

        ColOffsets[i].Name = ColX;
        GrDrawText(ColX, y, "Name", -1, *g_pGrTextMaterial);
        ColX += cxNameMax;

        ColOffsets[i].Score = ColX;
        GrDrawText(ColX, y, "Score", -1, *g_pGrTextMaterial);
        ColX += 50;

        if (GameType == RF_CTF)
        {
            ColOffsets[i].CtfFlags = ColX;
            GrDrawText(ColX, y, "Flags", -1, *g_pGrTextMaterial);
            ColX += 50;
        }

        ColOffsets[i].Ping = ColX;
        GrDrawText(ColX, y, "Ping", -1, *g_pGrTextMaterial);
    }
    y += 20;
    
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
        
        CEntity *pEntity = HandleToEntity(pPlayer->hEntity);
        static int GreenBm = BmLoad("DF_green.tga", -1, 0);
        static int RedBm = BmLoad("DF_red.tga", -1, 0);
        int StatusBm = pEntity ? GreenBm : RedBm;
        GrDrawImage(StatusBm, Offsets.StatusBm, RowY + 2, *g_pGrImageMaterial);

        CString strName, strNameNew;
        CString_InitFromStr(&strName, &pPlayer->strName);
        GrFitText(&strNameNew, strName, cxNameMax); // Note: this destroys strName
        GrDrawText(Offsets.Name, RowY, CString_CStr(&strNameNew), -1, *g_pGrTextMaterial);
        CString_Destroy(&strNameNew);
        
        sprintf(szBuf, "%hd", pPlayer->pStats->iScore);
        GrDrawText(Offsets.Score, RowY, szBuf, -1, *g_pGrTextMaterial);
        
        if (GameType == RF_CTF)
        {
            sprintf(szBuf, "%hu", pPlayer->pStats->cCaps);
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
