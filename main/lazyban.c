#include "lazyban.h"
#include "utils.h"
#include "rf.h"
#include "main.h"

void BanCmdHandlerHook(void)
{
    if(*g_pbNetworkGame && *g_pbLocalNetworkGame)
    {
        if(*g_pbCmdRun)
        {
            CPlayer *pPlayer;
            
            RfCmdGetNextArg(CMD_ARG_STR, 1);
            pPlayer = FindPlayer(g_pszCmdArg);
            if(pPlayer)
            {
                if(pPlayer != *g_ppLocalPlayer)
                {
                    RfConsolePrintf(g_ppszStringsTable[959], pPlayer->strName.psz);
                    RfBanIp(&(pPlayer->pNwData->Addr));
                    RfKickPlayer(pPlayer);
                } else
                    RfConsolePrintf("You cannot ban yourself!");
            }
            else
                RfConsolePrintf("Cannot find %s", g_pszCmdArg);
        }
        
        if(*g_pbCmdHelp)
        {
            RfConsoleWrite(g_ppszStringsTable[886], NULL);
            RfConsolePrintf("     ban <%s>", g_ppszStringsTable[835]);
        }
    }
}

void KickCmdHandlerHook(void)
{
    if(*g_pbNetworkGame && *g_pbLocalNetworkGame)
    {
        if(*g_pbCmdRun)
        {
            CPlayer *pPlayer;
            
            RfCmdGetNextArg(CMD_ARG_STR, 1);
            pPlayer = FindPlayer(g_pszCmdArg);
            if(pPlayer)
            {
                if(pPlayer != *g_ppLocalPlayer)
                {
                    RfConsolePrintf(g_ppszStringsTable[958], pPlayer->strName.psz);
                    RfKickPlayer(pPlayer);
                } else
                    RfConsolePrintf("You cannot kick yourself!");
            } else
                RfConsolePrintf("Cannot find %s", g_pszCmdArg);
        }
        
        if(*g_pbCmdHelp)
        {
            RfConsoleWrite(g_ppszStringsTable[886], NULL);
            RfConsolePrintf("     kick <%s>", g_ppszStringsTable[835]);
        }
    }
}

void UnbanLastCmdHandler(void)
{
    if(*g_pbNetworkGame && *g_pbLocalNetworkGame && *g_pbCmdRun)
    {
        BANLIST_ENTRY *pEntry = *g_ppBanlistLastEntry;
        if(pEntry != g_pBanlistNullEntry)
        {
            RfConsolePrintf("%s has been unbanned!", pEntry->szIp);
            pEntry->pNext->pPrev = pEntry->pPrev;
            pEntry->pPrev->pNext = pEntry->pPrev;
            RfDelete(pEntry);
        }
    }
}

void InitLazyban(void)
{
    WriteMemUInt8((PVOID)0x0047B6F0, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0047B6F1, ((ULONG_PTR)BanCmdHandlerHook) - (0x0047B6F0 + 0x5));
    WriteMemUInt8((PVOID)0x0047B580, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0047B581, ((ULONG_PTR)KickCmdHandlerHook) - (0x0047B580 + 0x5));
    
}
