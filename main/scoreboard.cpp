#include "stdafx.h"
#include "scoreboard.h"
#include "utils.h"
#include "rf.h"
#include "rfproto.h"

using namespace rf;

static int g_ScoreRflogoTexture = 0;
static int g_HudFlagRedTexture = 0;
static int g_HudFlagBlueTexture = 0;
static bool g_ScoreboardHidden = false;

static int ScoreboardSortFunc(const void *Ptr1, const void *Ptr2)
{
    CPlayer *pPlayer1 = *((CPlayer**)Ptr1), *pPlayer2 = *((CPlayer**)Ptr2);
    return pPlayer2->pStats->iScore - pPlayer1->pStats->iScore;
}

static void DrawScoreboardInternalHook(BOOL bDraw)
{
    if (g_ScoreboardHidden) return;
    if (bDraw)
    {
        unsigned i, x, x2, y, y2;
        unsigned cx, cy, cxNameMax, cPlayers;
        unsigned cLeftCol = 0, cRightCol = 0;
        CPlayer *pPlayer;
        char szBuf[512];
        unsigned GameType, RedScore = 0, BlueScore = 0;
        CPlayer *Players[32];
        CString strName, strNameNew;
        
        GameType = GetGameType();
        
        pPlayer = *g_ppPlayersList;
        cPlayers = 0;
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
        
        cx = (GameType == RF_DM) ? 450 : 600;
        cy = ((GameType == RF_DM) ? 110 : 170) + std::max(cLeftCol, cRightCol) * 15;
        cxNameMax = cx / (GameType == RF_DM ? 1 : 2) - 25 - 50 * (GameType == RF_CTF ? 3 : 2);
        
        x = (GrGetMaxWidth() - cx) / 2;
        y = (GrGetMaxHeight() - cy) / 2;
        cLeftCol = cRightCol = 0;
        
        GrSetColor(0, 0, 0, 0x80);
        GrDrawRect(x, y, cx, cy, *g_pGrRectMaterial);
        
        /* Draw RF logo */
        GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        if (!g_ScoreRflogoTexture)
            g_ScoreRflogoTexture = BmLoad("score_rflogo.tga", -1, TRUE);
        GrDrawImage(g_ScoreRflogoTexture, x + cx / 2 - 170, y + 10, *g_pGrImageMaterial);
        
        /* Draw map and server name */
        GrSetColor(0xB0, 0xB0, 0xB0, 0xFF);
        sprintf(szBuf, "%s (%s) by %s", g_pstrLevelName->psz, g_pstrLevelFilename->psz, g_pstrLevelAuthor->psz);
        GrDrawAlignedText(1, x + cx/2, y + 45, szBuf, -1, *g_pGrTextMaterial);
        i = sprintf(szBuf, "%s (", g_pstrServName->psz);
        NwAddrToStr(szBuf + i, sizeof(szBuf) - i, g_pServAddr);
        i += strlen(szBuf + i);
        sprintf(szBuf + i, ")");
        GrDrawAlignedText(1, x + cx/2, y + 60, szBuf, -1, *g_pGrTextMaterial);
        y += 80;
        
        /* Draw team scores */
        if (GameType == RF_CTF)
        {
            if (!g_HudFlagRedTexture)
                g_HudFlagRedTexture = BmLoad("hud_flag_red.tga", -1, TRUE);
            if (!g_HudFlagBlueTexture)
                g_HudFlagBlueTexture = BmLoad("hud_flag_blue.tga", -1, TRUE);
            GrDrawImage(g_HudFlagRedTexture, x + cx * 2 / 6, y, *g_pGrImageMaterial);
            GrDrawImage(g_HudFlagBlueTexture, x + cx * 4 / 6, y, *g_pGrImageMaterial);
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
            GrDrawAlignedText(1, x + cx * 1 / 6, y, szBuf, *g_pBigFontId, *g_pGrTextMaterial);
            GrSetColor(0x20, 0x20, 0xD0, 0xFF);
            sprintf(szBuf, "%u", BlueScore);
            GrDrawAlignedText(1, x + cx * 5 / 6, y, szBuf, *g_pBigFontId, *g_pGrTextMaterial);
            y += 60;
        }
        
        /* Draw headers */
        GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        for (i = 0; i < (GameType == RF_DM ? 1u : 2u); ++i)
        {
            x2 = x + i*300 + 13 + 12;
            GrDrawText(x2, y, "Name", -1, *g_pGrTextMaterial);
            x2 += cxNameMax;
            GrDrawText(x2, y, "Score", -1, *g_pGrTextMaterial);
            x2 += 50;
            if (GameType == RF_CTF)
            {
                GrDrawText(x2, y, "Flags", -1, *g_pGrTextMaterial);
                x2 += 50;
            }
            GrDrawText(x2, y, "Ping", -1, *g_pGrTextMaterial);
        }
        y += 20;
        
        /* Finally draw the list */
        for (i = 0; i < cPlayers; ++i)
        {
            pPlayer = Players[i];
            
            if (GameType == RF_DM || !pPlayer->bBlueTeam)
            {
                x2 = x + 13;
                y2 = y + cLeftCol * 15;
                ++cLeftCol;
            }
            else
            {
                x2 = x + 300 + 13;
                y2 = y + cRightCol * 15;
                ++cRightCol;
            }
            
            if (pPlayer == *g_ppPlayersList)
                GrSetColor(0xFF, 0xFF, 0x80, 0xFF);
            else
                GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
            
            CEntity *pEntity = HandleToEntity(pPlayer->hEntity);
            static int GreenBm = BmLoad("DF_green.tga", -1, 0);
            static int RedBm = BmLoad("DF_red.tga", -1, 0);
            int StatusBm = pEntity ? GreenBm : RedBm;
            GrDrawImage(StatusBm, x2, y2+2, *g_pGrImageMaterial);
            x2 += 12;

            strName.psz = StringAlloc(pPlayer->strName.cch + 1);
            strcpy(strName.psz, pPlayer->strName.psz);
            strName.cch = pPlayer->strName.cch;
            GrFitText(&strNameNew, strName, cxNameMax);
            GrDrawText(x2, y2, strNameNew.psz, -1, *g_pGrTextMaterial);
            x2 += cxNameMax;
            StringFree(strNameNew.psz);
            
            sprintf(szBuf, "%hd", pPlayer->pStats->iScore);
            GrDrawText(x2, y2, szBuf, -1, *g_pGrTextMaterial);
            x2 += 50;
            if (GameType == RF_CTF)
            {
                sprintf(szBuf, "%hu", pPlayer->pStats->cCaps);
                GrDrawText(x2, y2, szBuf, -1, *g_pGrTextMaterial);
                x2 += 50;
            }
            
            if (pPlayer->pNwData)
            {
                sprintf(szBuf, "%hu", pPlayer->pNwData->dwPing);
                GrDrawText(x2, y2, szBuf, -1, *g_pGrTextMaterial);
            }
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