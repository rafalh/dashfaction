#include "stdafx.h"
#include "kill.h"
#include "rf.h"
#include "utils.h"
#include <ShortTypes.h>
#include <FunHook2.h>

void KillInitPlayer(rf::Player* player)
{
    auto stats = reinterpret_cast<PlayerStatsNew*>(player->pStats);
    stats->cKills = 0;
    stats->cDeaths = 0;
}

FunHook2<void()> MpResetNetGame_Hook{
    0x0046E450,
    []() {
        rf::Player* player = rf::g_pPlayersList;
        while (player) {
            KillInitPlayer(player);
            player = player->pNext;

            if (player == rf::g_pPlayersList)
                break;
        }
        MpResetNetGame_Hook.CallTarget();
    }
};

static const char *NullToEmpty(const char* str)
{
    return str ? str : "";
}

void OnPlayerKill(rf::Player *killed_player, rf::Player *killer_player)
{
    rf::String msg;
    const char *mui_msg;
    rf::ChatMsgColor color_id;

    rf::EntityObj* killer_entity = killer_player ? rf::EntityGetFromHandle(killer_player->hEntity) : NULL;

    if (!killer_player) {
        color_id = rf::ChatMsgColor::default_;
        mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_WAS_KILLED_MYSTERIOUSLY]);
        msg = rf::String::Format("%s%s", killed_player->strName.CStr(), mui_msg);
    }
    else if (killed_player == rf::g_pLocalPlayer) {
        color_id = rf::ChatMsgColor::white_white;
        if (killer_player == killed_player) {
            mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_YOU_KILLED_YOURSELF]);
            msg = rf::String::Format("%s", mui_msg);
        }
        else if (killer_entity && killer_entity->WeaponInfo.WeaponClsId == rf::g_RiotStickClsId) {
            mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_YOU_JUST_GOT_BEAT_DOWN_BY]);
            msg = rf::String::Format("%s%s!", mui_msg, killer_player->strName.CStr());
        }
        else {
            mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_YOU_WERE_KILLED_BY]);

            const char* weapon_name = nullptr;
            int killer_weapon_cls_id = killer_entity ? killer_entity->WeaponInfo.WeaponClsId : -1;
            if (killer_weapon_cls_id >= 0 && killer_weapon_cls_id < 64) {
                auto &weapon_cls = rf::g_pWeaponClasses[killer_weapon_cls_id];
                weapon_name = weapon_cls.strDisplayName.CStr();
            }

            auto killer_name = killer_player->strName.CStr();
            if (weapon_name)
                msg = rf::String::Format("%s%s (%s)!", mui_msg, killer_name, weapon_name);
            else
                msg = rf::String::Format("%s%s!", mui_msg, killer_name);
        }
    }
    else if (killer_player == rf::g_pLocalPlayer) {
        color_id = rf::ChatMsgColor::white_white;
        mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_YOU_KILLED]);
        msg = rf::String::Format("%s%s!", mui_msg, killed_player->strName.CStr());
    }
    else {
        color_id = rf::ChatMsgColor::default_;
        if (killer_player == killed_player) {
            mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_WAS_KILLED_BY_HIW_OWN_HAND]);
            msg = rf::String::Format("%s%s", killed_player->strName.CStr(), mui_msg);
        }
        else {
            if (killer_entity && killer_entity->WeaponInfo.WeaponClsId == rf::g_RiotStickClsId)
                mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_GOT_BEAT_DOWN_BY]);
            else
                mui_msg = NullToEmpty(rf::g_ppszStringsTable[rf::STR_WAS_KILLED_BY]);
            msg = rf::String::Format("%s%s%s", killed_player->strName.CStr(), mui_msg, killer_player->strName.CStr());
        }
    }

    rf::String prefix;
    rf::ChatPrint(msg, color_id, prefix);

    auto killed_stats = reinterpret_cast<PlayerStatsNew*>(killed_player->pStats);
    ++killed_stats->cDeaths;

    if (killer_player) {
        auto killer_stats = reinterpret_cast<PlayerStatsNew*>(killer_player->pStats);
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
    WriteMem<i8>(0x004A33B5 + 1, sizeof(PlayerStatsNew));
    MpResetNetGame_Hook.Install();
}
