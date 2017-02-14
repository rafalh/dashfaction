#include "stdafx.h"
#include "kill.h"
#include "rf.h"
#include "utils.h"
#include "scoreboard.h"

using namespace rf;

auto g_MpResetNetGameHook = makeFunHook(MpResetNetGame);

void KillInitPlayer(CPlayer *pPlayer)
{
    auto pStats = (PlayerStatsNew*)pPlayer->pStats;
    pStats->cKills = 0;
    pStats->cDeaths = 0;
}

void MpResetNetGame_New()
{
    CPlayer *pPlayer = *g_ppPlayersList;
    while (pPlayer)
    {
        KillInitPlayer(pPlayer);
        pPlayer = pPlayer->pNext;
        if (pPlayer == *g_ppPlayersList) break;
    }
    g_MpResetNetGameHook.callTrampoline();
}

void OnPlayerKill(CPlayer *pKilled, CPlayer *pKiller)
{
    CString strMsg, strPrefix, *pstrWeaponName;
    const char *pszMuiMsg, *pszWeaponName = NULL;
    unsigned ColorId;
    CEntity *pKillerEntity;
    
    pKillerEntity = pKiller ? HandleToEntity(pKiller->hEntity) : NULL;
    
    if (!pKiller)
    {
        ColorId = 5;
        pszMuiMsg = g_ppszStringsTable[STR_WAS_KILLED_MYSTERIOUSLY] ? g_ppszStringsTable[STR_WAS_KILLED_MYSTERIOUSLY] : "";
        strMsg.psz = StringAlloc(pKilled->strName.cch + strlen(pszMuiMsg) + 1);
        strMsg.cch = sprintf(strMsg.psz, "%s%s", pKilled->strName.psz, pszMuiMsg);
    }
    else if (pKilled == *g_ppLocalPlayer)
    {
        ColorId = 4;
        if (pKiller == pKilled)
        {
            pszMuiMsg = g_ppszStringsTable[STR_YOU_KILLED_YOURSELF] ? g_ppszStringsTable[STR_YOU_KILLED_YOURSELF] : "";
            strMsg.psz = StringAlloc(strlen(pszMuiMsg) + 1);
            strMsg.cch = sprintf(strMsg.psz, "%s", pszMuiMsg);
        }
        else
        {
            unsigned cchWeaponName = 0;
            
            if (pKillerEntity && pKillerEntity->WeaponSel.WeaponClsId == *g_pRiotStickClassId)
            {
                pszMuiMsg = g_ppszStringsTable[STR_YOU_JUST_GOT_BEAT_DOWN_BY] ? g_ppszStringsTable[STR_YOU_JUST_GOT_BEAT_DOWN_BY] : "";
                strMsg.psz = StringAlloc(strlen(pszMuiMsg) + pKiller->strName.cch + 2);
                strMsg.cch = sprintf(strMsg.psz, "%s%s!", pszMuiMsg, pKiller->strName.psz);
            }
            else
            {
                pszMuiMsg = g_ppszStringsTable[STR_YOU_WERE_KILLED_BY] ? g_ppszStringsTable[STR_YOU_WERE_KILLED_BY] : "";
                
                if (pKillerEntity && pKillerEntity->WeaponSel.WeaponClsId >= 0 && pKillerEntity->WeaponSel.WeaponClsId < 64)
                {
                    pstrWeaponName = &g_pWeaponClasses[pKillerEntity->WeaponSel.WeaponClsId].strDisplayName;
                    if (pstrWeaponName->psz && pstrWeaponName->cch)
                    {
                        pszWeaponName = pstrWeaponName->psz;
                        cchWeaponName = pstrWeaponName->cch;
                    }
                }
                
                strMsg.psz = StringAlloc(strlen(pszMuiMsg) + pKiller->strName.cch + 3 + cchWeaponName + 2);
                strMsg.cch = sprintf(strMsg.psz, "%s%s%s%s%s!",
                                     pszMuiMsg,
                                     pKiller->strName.psz,
                                     pszWeaponName ? " (" : "",
                                     pszWeaponName ? pszWeaponName : "",
                                     pszWeaponName ? ")" : "");
            }
        }
    }
    else if (pKiller == *g_ppLocalPlayer)
    {
        ColorId = 4;
        pszMuiMsg = g_ppszStringsTable[STR_YOU_KILLED] ? g_ppszStringsTable[STR_YOU_KILLED] : "";
        strMsg.psz = StringAlloc(strlen(pszMuiMsg) + pKilled->strName.cch + 2);
        strMsg.cch = sprintf(strMsg.psz, "%s%s!", pszMuiMsg, pKilled->strName.psz);
    }
    else
    {
        ColorId = 5;
        if (pKiller == pKilled)
        {
            pszMuiMsg = g_ppszStringsTable[STR_WAS_KILLED_BY_HIW_OWN_HAND] ? g_ppszStringsTable[STR_WAS_KILLED_BY_HIW_OWN_HAND] : "";
            strMsg.psz = StringAlloc(pKilled->strName.cch + strlen(pszMuiMsg) + 1);
            strMsg.cch = sprintf(strMsg.psz, "%s%s", pKilled->strName.psz, pszMuiMsg);
        }
        else
        {
            if (pKillerEntity && pKillerEntity->WeaponSel.WeaponClsId == *g_pRiotStickClassId)
                pszMuiMsg = g_ppszStringsTable[STR_GOT_BEAT_DOWN_BY] ? g_ppszStringsTable[STR_GOT_BEAT_DOWN_BY] : "";
            else
                pszMuiMsg = g_ppszStringsTable[STR_WAS_KILLED_BY] ? g_ppszStringsTable[STR_WAS_KILLED_BY] : "";
            strMsg.psz = StringAlloc(pKilled->strName.cch + strlen(pszMuiMsg) + pKiller->strName.cch + 1);
            strMsg.cch = sprintf(strMsg.psz, "%s%s%s", pKilled->strName.psz, pszMuiMsg, pKiller->strName.psz);
        }
    }
    
    strPrefix.psz = NULL;
    strPrefix.cch = 0;
    ChatPrint(strMsg, ColorId, strPrefix);
    
    auto pKilledStats = (PlayerStatsNew*)pKilled->pStats;
    ++pKilledStats->cDeaths;

    if (pKiller)
    {
        auto pKillerStats = (PlayerStatsNew*)pKiller->pStats;
        if (pKiller != pKilled)
        {
            ++pKillerStats->iScore;
            ++pKillerStats->cKills;
        }
        else
            --pKillerStats->iScore;
    }
}

void InitKill(void)
{
    // Player kill handling
    WriteMemUInt8(0x00420703, ASM_PUSH_EBX);
    WriteMemUInt8(0x00420704, ASM_PUSH_EDI);
    WriteMemUInt8(0x00420705, ASM_LONG_CALL_REL);
    WriteMemInt32(0x00420705 + 1, (uintptr_t)OnPlayerKill - (0x00420705 + 0x5));
    WriteMemUInt16(0x0042070A, ASM_ADD_ESP_BYTE);
    WriteMemUInt8(0x0042070C, 8);
    WriteMemUInt8(0x0042070D, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0042070D + 1, 0x00420B03 - (0x0042070D + 0x5));

    // Change player stats structure
    WriteMemInt8(0x004A33B5 + 1, sizeof(PlayerStatsNew));
    g_MpResetNetGameHook.hook(MpResetNetGame_New);
}
