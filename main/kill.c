#include "kill.h"
#include "rf.h"
#include "utils.h"

void OnPlayerKill(CPlayer *pKilled, CPlayer *pKiller)
{
    CString strMsg, strPrefix, *pstrWeaponName;
    const char *pszMuiMsg, *pszWeaponName = NULL;
    unsigned ColorId;
    CEntity *pKillerEntity;
    
    pKillerEntity = pKiller ? HandleToEntity(pKiller->hEntity) : NULL;
    
    if(!pKiller)
    {
        ColorId = 5;
        pszMuiMsg = g_ppszStringsTable[695] ? g_ppszStringsTable[695] : ""; // was killed mysteriously
        strMsg.psz = StringAlloc(pKilled->strName.cch + strlen(pszMuiMsg) + 1);
        strMsg.cch = sprintf(strMsg.psz, "%s%s", pKilled->strName.psz, pszMuiMsg);
    }
    else if(pKilled == *g_ppLocalPlayer)
    {
        ColorId = 4;
        if(pKiller == pKilled)
        {
            pszMuiMsg = g_ppszStringsTable[942] ? g_ppszStringsTable[942] : ""; // You killed yourself!
            strMsg.psz = StringAlloc(strlen(pszMuiMsg) + 1);
            strMsg.cch = sprintf(strMsg.psz, "%s", pszMuiMsg);
        }
        else
        {
            unsigned cchWeaponName = 0;
            
            if(pKillerEntity && pKillerEntity->WeaponClassId == *g_pRiotStickClassId)
            {
                pszMuiMsg = g_ppszStringsTable[943] ? g_ppszStringsTable[943] : ""; // You just got beat down by
                strMsg.psz = StringAlloc(strlen(pszMuiMsg) + pKiller->strName.cch + 2);
                strMsg.cch = sprintf(strMsg.psz, "%s%s!", pszMuiMsg, pKiller->strName.psz);
            }
            else
            {
                pszMuiMsg = g_ppszStringsTable[944] ? g_ppszStringsTable[944] : ""; // You were killed by
                
                if(pKillerEntity && pKillerEntity->WeaponClassId >= 0 && pKillerEntity->WeaponClassId < 64)
                {
                    pstrWeaponName = &g_pWeaponClasses[pKillerEntity->WeaponClassId].strDisplayName;
                    if(pstrWeaponName->psz && pstrWeaponName->cch)
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
    else if(pKiller == *g_ppLocalPlayer)
    {
        ColorId = 4;
        pszMuiMsg = g_ppszStringsTable[945] ? g_ppszStringsTable[945] : ""; // You killed
        strMsg.psz = StringAlloc(strlen(pszMuiMsg) + pKilled->strName.cch + 2);
        strMsg.cch = sprintf(strMsg.psz, "%s%s!", pszMuiMsg, pKilled->strName.psz);
    }
    else
    {
        ColorId = 5;
        if(pKiller == pKilled)
        {
            pszMuiMsg = g_ppszStringsTable[693] ? g_ppszStringsTable[693] : ""; // was killed by his own hand
            strMsg.psz = StringAlloc(pKilled->strName.cch + strlen(pszMuiMsg) + 1);
            strMsg.cch = sprintf(strMsg.psz, "%s%s", pKilled->strName.psz, pszMuiMsg);
        }
        else
        {
            if(pKillerEntity && pKillerEntity->WeaponClassId == *g_pRiotStickClassId)
                pszMuiMsg = g_ppszStringsTable[946] ? g_ppszStringsTable[946] : ""; // got beat down by
            else
                pszMuiMsg = g_ppszStringsTable[694] ? g_ppszStringsTable[694] : ""; // was killed by
            strMsg.psz = StringAlloc(pKilled->strName.cch + strlen(pszMuiMsg) + pKiller->strName.cch + 1);
            strMsg.cch = sprintf(strMsg.psz, "%s%s%s", pKilled->strName.psz, pszMuiMsg, pKiller->strName.psz);
        }
    }
    
    strPrefix.psz = NULL;
    strPrefix.cch = 0;
    ChatPrint(strMsg, ColorId, strPrefix);
    
    if(pKiller)
    {
        if(pKiller != pKilled)
            ++pKiller->pStats->iScore;
        else
            --pKiller->pStats->iScore;
    }
}

void InitKill(void)
{
    /* Player kill handling */
    WriteMemUInt8((PVOID)0x00420703, ASM_PUSH_EBX);
    WriteMemUInt8((PVOID)0x00420704, ASM_PUSH_EDI);
    WriteMemUInt8((PVOID)0x00420705, ASM_LONG_CALL_REL);
    WriteMemUInt32((PVOID)0x00420706, ((ULONG_PTR)OnPlayerKill) - (0x00420705 + 0x5));
    WriteMemUInt16((PVOID)0x0042070A, ASM_ADD_ESP_BYTE);
    WriteMemUInt8((PVOID)0x0042070C, 8);
    WriteMemUInt8((PVOID)0x0042070D, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0042070E, 0x00420B03 - (0x0042070D + 0x5));
}
