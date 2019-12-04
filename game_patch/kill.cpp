#include "kill.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include <patch_common/FunHook.h>
#include <patch_common/ShortTypes.h>

void KillInitPlayer(rf::Player* player)
{
    auto stats = reinterpret_cast<PlayerStatsNew*>(player->stats);
    stats->num_kills = 0;
    stats->num_deaths = 0;
}

FunHook<void()> MpResetNetGame_hook{
    0x0046E450,
    []() {
        auto player_list = SinglyLinkedList{rf::player_list};
        for (auto& player : player_list) {
            KillInitPlayer(&player);
        }
        MpResetNetGame_hook.CallTarget();
    },
};

static const char* NullToEmpty(const char* str)
{
    return str ? str : "";
}

void OnPlayerKill(rf::Player* killed_player, rf::Player* killer_player)
{
    rf::String msg;
    const char* mui_msg;
    rf::ChatMsgColor color_id;

    rf::EntityObj* killer_entity = killer_player ? rf::EntityGetFromHandle(killer_player->entity_handle) : nullptr;

    if (!killer_player) {
        color_id = rf::ChatMsgColor::default_;
        mui_msg = NullToEmpty(rf::strings::was_killed_mysteriously);
        msg = rf::String::Format("%s%s", killed_player->name.CStr(), mui_msg);
    }
    else if (killed_player == rf::local_player) {
        color_id = rf::ChatMsgColor::white_white;
        if (killer_player == killed_player) {
            mui_msg = NullToEmpty(rf::strings::you_killed_yourself);
            msg = rf::String::Format("%s", mui_msg);
        }
        else if (killer_entity && killer_entity->weapon_info.weapon_cls_id == rf::riot_stick_cls_id) {
            mui_msg = NullToEmpty(rf::strings::you_just_got_beat_down_by);
            msg = rf::String::Format("%s%s!", mui_msg, killer_player->name.CStr());
        }
        else {
            mui_msg = NullToEmpty(rf::strings::you_were_killed_by);

            const char* weapon_name = nullptr;
            int killer_weapon_cls_id = killer_entity ? killer_entity->weapon_info.weapon_cls_id : -1;
            if (killer_weapon_cls_id >= 0 && killer_weapon_cls_id < 64) {
                auto& weapon_cls = rf::weapon_classes[killer_weapon_cls_id];
                weapon_name = weapon_cls.display_name.CStr();
            }

            auto killer_name = killer_player->name.CStr();
            if (weapon_name)
                msg = rf::String::Format("%s%s (%s)!", mui_msg, killer_name, weapon_name);
            else
                msg = rf::String::Format("%s%s!", mui_msg, killer_name);
        }
    }
    else if (killer_player == rf::local_player) {
        color_id = rf::ChatMsgColor::white_white;
        mui_msg = NullToEmpty(rf::strings::you_killed);
        msg = rf::String::Format("%s%s!", mui_msg, killed_player->name.CStr());
    }
    else {
        color_id = rf::ChatMsgColor::default_;
        if (killer_player == killed_player) {
            mui_msg = NullToEmpty(rf::strings::was_killed_by_his_own_hand);
            msg = rf::String::Format("%s%s", killed_player->name.CStr(), mui_msg);
        }
        else {
            if (killer_entity && killer_entity->weapon_info.weapon_cls_id == rf::riot_stick_cls_id)
                mui_msg = NullToEmpty(rf::strings::got_beat_down_by);
            else
                mui_msg = NullToEmpty(rf::strings::was_killed_by);
            msg = rf::String::Format("%s%s%s", killed_player->name.CStr(), mui_msg, killer_player->name.CStr());
        }
    }

    rf::String prefix;
    rf::ChatPrint(msg, color_id, prefix);

    auto killed_stats = reinterpret_cast<PlayerStatsNew*>(killed_player->stats);
    ++killed_stats->num_deaths;

    if (killer_player) {
        auto killer_stats = reinterpret_cast<PlayerStatsNew*>(killer_player->stats);
        if (killer_player != killed_player) {
            ++killer_stats->score;
            ++killer_stats->num_kills;
        }
        else
            --killer_stats->score;
    }
}

FunHook<void(rf::EntityObj*)> EntityOnDeath_hook{
    0x0041FDC0,
    [](rf::EntityObj* entity) {
        // Reset fpgun animation when player dies
        if (rf::local_player && entity->_super.handle == rf::local_player->entity_handle && rf::local_player->fpgun_mesh) {
            auto AnimMeshResetAction = AddrAsRef<void(rf::AnimMesh*)>(0x00503400);
            auto FpgunStopActionSound = AddrAsRef<void(rf::Player*)>(0x004A9490);
            AnimMeshResetAction(rf::local_player->fpgun_mesh);
            FpgunStopActionSound(rf::local_player);
        }
        EntityOnDeath_hook.CallTarget(entity);
    },
};

void InitKill()
{
    // Player kill handling
    using namespace asm_regs;
    AsmWriter(0x00420703)
        .push(ebx)
        .push(edi)
        .call(OnPlayerKill)
        .add(esp, 8)
        .jmp(0x00420B03);

    // Change player stats structure
    WriteMem<i8>(0x004A33B5 + 1, sizeof(PlayerStatsNew));
    MpResetNetGame_hook.Install();

    // Reset fpgun animation when player dies
    EntityOnDeath_hook.Install();
}
