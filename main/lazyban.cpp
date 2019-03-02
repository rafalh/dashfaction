#include "stdafx.h"
#include "lazyban.h"
#include "utils.h"
#include "rf.h"
#include "commands.h"

namespace rf {
struct BanlistEntry {
    char ip_addr[24];
    BanlistEntry* next;
    BanlistEntry* prev;
};

static auto &banlist_first_entry = *(BanlistEntry**)0x0064EC20;
static auto &banlist_last_entry = *(BanlistEntry**)0x0064EC24;
static auto &banlist_null_entry = *(BanlistEntry*)0x0064EC08;
}

void BanCmdHandlerHook()
{
    if (rf::g_bNetworkGame && rf::g_bLocalNetworkGame) {
        if (rf::g_bDcRun) {
            rf::DcGetArg(rf::DC_ARG_STR, 1);
            rf::Player* player = FindBestMatchingPlayer(rf::g_pszDcArg);

            if (player) {
                if (player != rf::g_pLocalPlayer) {
                    rf::DcPrintf(rf::g_ppszStringsTable[959], player->strName.CStr());
                    rf::BanIp(&(player->pNwData->Addr));
                    rf::KickPlayer(player);
                } else
                    rf::DcPrintf("You cannot ban yourself!");
            }
        }

        if (rf::g_bDcHelp) {
            rf::DcPrint(rf::g_ppszStringsTable[rf::STR_USAGE], NULL);
            rf::DcPrintf("     ban <%s>", rf::g_ppszStringsTable[rf::STR_PLAYER_NAME]);
        }
    }
}

void KickCmdHandlerHook()
{
    if (rf::g_bNetworkGame && rf::g_bLocalNetworkGame) {
        if (rf::g_bDcRun) {
            rf::DcGetArg(rf::DC_ARG_STR, 1);
            rf::Player* player = FindBestMatchingPlayer(rf::g_pszDcArg);

            if (player) {
                if (player != rf::g_pLocalPlayer) {
                    rf::DcPrintf(rf::g_ppszStringsTable[rf::STR_KICKING_PLAYER], player->strName.CStr());
                    rf::KickPlayer(player);
                } else
                    rf::DcPrintf("You cannot kick yourself!");
            }
        }

        if (rf::g_bDcHelp) {
            rf::DcPrint(rf::g_ppszStringsTable[rf::STR_USAGE], NULL);
            rf::DcPrintf("     kick <%s>", rf::g_ppszStringsTable[rf::STR_PLAYER_NAME]);
        }
    }
}

void UnbanLastCmdHandler()
{
    if (rf::g_bNetworkGame && rf::g_bLocalNetworkGame && rf::g_bDcRun) {
        rf::BanlistEntry* entry = rf::banlist_last_entry;
        if (entry != &rf::banlist_null_entry) {
            rf::DcPrintf("%s has been unbanned!", entry->ip_addr);
            entry->next->prev = entry->prev;
            entry->prev->next = entry->next;
            rf::Free(entry);
        }
    }
}

void InitLazyban()
{
    AsmWritter(0x0047B6F0).jmpLong(BanCmdHandlerHook);
    AsmWritter(0x0047B580).jmpLong(KickCmdHandlerHook);
}
