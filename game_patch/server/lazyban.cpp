#include "server_internal.h"
#include "../console/console.h"
#include "../rf/network.h"
#include "../rf/debug_console.h"
#include "../rf/player.h"
#include <patch_common/AsmWriter.h>
#include <patch_common/CallHook.h>

namespace rf
{
struct BanlistEntry
{
    char ip_addr[24];
    BanlistEntry* next;
    BanlistEntry* prev;
};

static auto& banlist_first_entry = AddrAsRef<BanlistEntry*>(0x0064EC20);
static auto& banlist_last_entry = AddrAsRef<BanlistEntry*>(0x0064EC24);
static auto& banlist_null_entry = AddrAsRef<BanlistEntry>(0x0064EC08);
} // namespace rf

std::vector<int> g_players_to_kick;

void KickPlayerDelayed(rf::Player* player)
{
    g_players_to_kick.push_back(player->nw_data->player_id);
}

CallHook<void(rf::Player*)> KickPlayer_hook{
    0x0047B9BD,
    KickPlayerDelayed,
};

void ProcessDelayedKicks()
{
    // Process kicks outside of packet processing loop to avoid crash when a player is suddenly destroyed (00479299)
    for (int player_id : g_players_to_kick) {
        auto player = rf::GetPlayerById(player_id);
        if (player) {
            rf::KickPlayer(player);
        }
    }
    g_players_to_kick.clear();
}

void BanCmdHandlerHook()
{
    if (rf::is_multi && rf::is_server) {
        if (rf::dc_run) {
            rf::DcGetArg(rf::DC_ARG_STR, true);
            rf::Player* player = FindBestMatchingPlayer(rf::dc_str_arg);

            if (player) {
                if (player != rf::local_player) {
                    rf::DcPrintf(rf::strings::banning_player, player->name.CStr());
                    rf::BanIp(player->nw_data->addr);
                    KickPlayerDelayed(player);
                }
                else
                    rf::DcPrintf("You cannot ban yourself!");
            }
        }

        if (rf::dc_help) {
            rf::DcPrint(rf::strings::usage, nullptr);
            rf::DcPrintf("     ban <%s>", rf::strings::player_name);
        }
    }
}

void KickCmdHandlerHook()
{
    if (rf::is_multi && rf::is_server) {
        if (rf::dc_run) {
            rf::DcGetArg(rf::DC_ARG_STR, 1);
            rf::Player* player = FindBestMatchingPlayer(rf::dc_str_arg);

            if (player) {
                if (player != rf::local_player) {
                    rf::DcPrintf(rf::strings::kicking_player, player->name.CStr());
                    KickPlayerDelayed(player);
                }
                else
                    rf::DcPrintf("You cannot kick yourself!");
            }
        }

        if (rf::dc_help) {
            rf::DcPrint(rf::strings::usage, nullptr);
            rf::DcPrintf("     kick <%s>", rf::strings::player_name);
        }
    }
}

DcCommand2 unban_last_cmd{
    "unban_last",
    []() {
        if (rf::is_multi && rf::is_server) {
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
    AsmWriter(0x0047B6F0).jmp(BanCmdHandlerHook);
    AsmWriter(0x0047B580).jmp(KickCmdHandlerHook);

    unban_last_cmd.Register();

    KickPlayer_hook.Install();
}
