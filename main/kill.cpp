#include "stdafx.h"
#include "kill.h"
#include "rf.h"
#include "utils.h"
#include "scoreboard.h"
#include <FunHook2.h>

void KillInitPlayer(rf::Player *player)
{
    auto stats = reinterpret_cast<PlayerStatsNew*>(player->pStats);
    stats->cKills = 0;
    stats->cDeaths = 0;
}

FunHook2<void()> MpResetNetGame_Hook{
    0x0046E450,
    []() {
        rf::Player *player = rf::g_pPlayersList;
        while (player) {
            KillInitPlayer(player);
            player = player->pNext;

            if (player == rf::g_pPlayersList)
                break;
        }
        MpResetNetGame_Hook.CallTarget();
    }
};

void OnPlayerKill(rf::Player *killed_player, rf::Player *killer_player)
{
    rf::String msg;
    const char *mui_msg, *weapon_name = NULL;
    unsigned color_id;
    
    rf::EntityObj *killer_entity = killer_player ? rf::EntityGetFromHandle(killer_player->hEntity) : NULL;
    
    if (!killer_player) {
        color_id = 5;
        mui_msg = rf::g_ppszStringsTable[rf::STR_WAS_KILLED_MYSTERIOUSLY] ? rf::g_ppszStringsTable[rf::STR_WAS_KILLED_MYSTERIOUSLY] : "";
        msg.psz = rf::StringAlloc(killed_player->strName.cch + strlen(mui_msg) + 1);
        msg.cch = sprintf(msg.psz, "%s%s", killed_player->strName.psz, mui_msg);
    }
    else if (killed_player == rf::g_pLocalPlayer) {
        color_id = 4;
        if (killer_player == killed_player) {
            mui_msg = rf::g_ppszStringsTable[rf::STR_YOU_KILLED_YOURSELF] ? rf::g_ppszStringsTable[rf::STR_YOU_KILLED_YOURSELF] : "";
            msg.psz = rf::StringAlloc(strlen(mui_msg) + 1);
            msg.cch = sprintf(msg.psz, "%s", mui_msg);
        }
        else {
            unsigned weapon_name_len = 0;
            
            if (killer_entity && killer_entity->WeaponInfo.WeaponClsId == rf::g_RiotStickClsId) {
                mui_msg = rf::g_ppszStringsTable[rf::STR_YOU_JUST_GOT_BEAT_DOWN_BY] ? rf::g_ppszStringsTable[rf::STR_YOU_JUST_GOT_BEAT_DOWN_BY] : "";
                msg.psz = rf::StringAlloc(strlen(mui_msg) + killer_player->strName.cch + 2);
                msg.cch = sprintf(msg.psz, "%s%s!", mui_msg, killer_player->strName.psz);
            }
            else {
                mui_msg = rf::g_ppszStringsTable[rf::STR_YOU_WERE_KILLED_BY] ? rf::g_ppszStringsTable[rf::STR_YOU_WERE_KILLED_BY] : "";
                
                if (killer_entity && killer_entity->WeaponInfo.WeaponClsId >= 0 && killer_entity->WeaponInfo.WeaponClsId < 64) {
                    rf::String *weapon_name_str = &rf::g_pWeaponClasses[killer_entity->WeaponInfo.WeaponClsId].strDisplayName;
                    if (weapon_name_str->psz && weapon_name_str->cch) {
                        weapon_name = weapon_name_str->psz;
                        weapon_name_len = weapon_name_str->cch;
                    }
                }
                
                msg.psz = rf::StringAlloc(strlen(mui_msg) + killer_player->strName.cch + 3 + weapon_name_len + 2);
                msg.cch = sprintf(msg.psz, "%s%s%s%s%s!",
                                     mui_msg,
                                     killer_player->strName.psz,
                                     weapon_name ? " (" : "",
                                     weapon_name ? weapon_name : "",
                                     weapon_name ? ")" : "");
            }
        }
    }
    else if (killer_player == rf::g_pLocalPlayer) {
        color_id = 4;
        mui_msg = rf::g_ppszStringsTable[rf::STR_YOU_KILLED] ? rf::g_ppszStringsTable[rf::STR_YOU_KILLED] : "";
        msg.psz = rf::StringAlloc(strlen(mui_msg) + killed_player->strName.cch + 2);
        msg.cch = sprintf(msg.psz, "%s%s!", mui_msg, killed_player->strName.psz);
    }
    else {
        color_id = 5;
        if (killer_player == killed_player) {
            mui_msg = rf::g_ppszStringsTable[rf::STR_WAS_KILLED_BY_HIW_OWN_HAND] ? rf::g_ppszStringsTable[rf::STR_WAS_KILLED_BY_HIW_OWN_HAND] : "";
            msg.psz = rf::StringAlloc(killed_player->strName.cch + strlen(mui_msg) + 1);
            msg.cch = sprintf(msg.psz, "%s%s", killed_player->strName.psz, mui_msg);
        }
        else {
            if (killer_entity && killer_entity->WeaponInfo.WeaponClsId == rf::g_RiotStickClsId)
                mui_msg = rf::g_ppszStringsTable[rf::STR_GOT_BEAT_DOWN_BY] ? rf::g_ppszStringsTable[rf::STR_GOT_BEAT_DOWN_BY] : "";
            else
                mui_msg = rf::g_ppszStringsTable[rf::STR_WAS_KILLED_BY] ? rf::g_ppszStringsTable[rf::STR_WAS_KILLED_BY] : "";
            msg.psz = rf::StringAlloc(killed_player->strName.cch + strlen(mui_msg) + killer_player->strName.cch + 1);
            msg.cch = sprintf(msg.psz, "%s%s%s", killed_player->strName.psz, mui_msg, killer_player->strName.psz);
        }
    }

    rf::String prefix;
    rf::String::InitEmpty(&prefix);
    rf::ChatPrint(msg, color_id, prefix);
    
    auto killed_stats = (PlayerStatsNew*)killed_player->pStats;
    ++killed_stats->cDeaths;

    if (killer_player) {
        auto killer_stats = (PlayerStatsNew*)killer_player->pStats;
        if (killer_player != killed_player) {
            ++killer_stats->iScore;
            ++killer_stats->cKills;
        }
        else
            --killer_stats->iScore;
    }
}

void InitKill()
{
    // Player kill handling
    AsmWritter(0x00420703)
        .push(AsmRegs::EBX)
        .push(AsmRegs::EDI)
        .callLong(OnPlayerKill)
        .addEsp(8)
        .jmpLong(0x00420B03);

    // Change player stats structure
    WriteMemInt8(0x004A33B5 + 1, sizeof(PlayerStatsNew));
    MpResetNetGame_Hook.Install();
}
