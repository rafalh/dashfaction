#include "lazyban.h"
#include "commands.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"

namespace rf
{
struct BanlistEntry
{
    char ip_addr[24];
    BanlistEntry* next;
    BanlistEntry* prev;
};

static auto& banlist_first_entry = *(BanlistEntry**)0x0064EC20;
static auto& banlist_last_entry = *(BanlistEntry**)0x0064EC24;
static auto& banlist_null_entry = *(BanlistEntry*)0x0064EC08;
} // namespace rf

void BanCmdHandlerHook()
{
    if (rf::g_IsNetworkGame && rf::g_IsLocalNetworkGame) {
        if (rf::g_DcRun) {
            rf::DcGetArg(rf::DC_ARG_STR, 1);
            rf::Player* player = FindBestMatchingPlayer(rf::g_DcStrArg);

            if (player) {
                if (player != rf::g_LocalPlayer) {
                    rf::DcPrintf(rf::strings::array[959], player->strName.CStr());
                    rf::BanIp(player->NwData->Addr);
                    rf::KickPlayer(player);
                }
                else
                    rf::DcPrintf("You cannot ban yourself!");
            }
        }

        if (rf::g_DcHelp) {
            rf::DcPrint(rf::strings::usage, nullptr);
            rf::DcPrintf("     ban <%s>", rf::strings::player_name);
        }
    }
}

void KickCmdHandlerHook()
{
    if (rf::g_IsNetworkGame && rf::g_IsLocalNetworkGame) {
        if (rf::g_DcRun) {
            rf::DcGetArg(rf::DC_ARG_STR, 1);
            rf::Player* player = FindBestMatchingPlayer(rf::g_DcStrArg);

            if (player) {
                if (player != rf::g_LocalPlayer) {
                    rf::DcPrintf(rf::strings::kicking_player, player->strName.CStr());
                    rf::KickPlayer(player);
                }
                else
                    rf::DcPrintf("You cannot kick yourself!");
            }
        }

        if (rf::g_DcHelp) {
            rf::DcPrint(rf::strings::usage, nullptr);
            rf::DcPrintf("     kick <%s>", rf::strings::player_name);
        }
    }
}

DcCommand2 unban_last_cmd{
    "unban_last",
    []() {
        if (rf::g_IsNetworkGame && rf::g_IsLocalNetworkGame) {
            rf::BanlistEntry* entry = rf::banlist_last_entry;
            if (entry != &rf::banlist_null_entry) {
                rf::DcPrintf("%s has been unbanned!", entry->ip_addr);
                entry->next->prev = entry->prev;
                entry->prev->next = entry->next;
                rf::Free(entry);
            }
        }
    },
    "Unbans last banned player",
};

void InitLazyban()
{
    AsmWritter(0x0047B6F0).jmp(BanCmdHandlerHook);
    AsmWritter(0x0047B580).jmp(KickCmdHandlerHook);

    unban_last_cmd.Register();
}
