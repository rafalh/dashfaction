#include "multi.h"
#include "../rf/player.h"
#include "../rf/entity.h"
#include "../rf/localize.h"
#include "../rf/multi.h"
#include "../rf/weapon.h"
#include "../utils/list-utils.h"
#include <patch_common/FunHook.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>

void player_fpgun_on_player_death(rf::Player* pp);

void multi_kill_init_player(rf::Player* player)
{
    auto stats = reinterpret_cast<PlayerStatsNew*>(player->stats);
    stats->num_kills = 0;
    stats->num_deaths = 0;
}

FunHook<void()> multi_level_init_hook{
    0x0046E450,
    []() {
        auto player_list = SinglyLinkedList{rf::player_list};
        for (auto& player : player_list) {
            multi_kill_init_player(&player);
        }
        multi_level_init_hook.call_target();
    },
};

static const char* null_to_empty(const char* str)
{
    return str ? str : "";
}

void on_player_kill(rf::Player* killed_player, rf::Player* killer_player)
{
    rf::String msg;
    const char* mui_msg;
    rf::ChatMsgColor color_id;

    rf::Entity* killer_entity = killer_player ? rf::entity_from_handle(killer_player->entity_handle) : nullptr;

    if (!killer_player) {
        color_id = rf::ChatMsgColor::default_;
        mui_msg = null_to_empty(rf::strings::was_killed_mysteriously);
        msg = rf::String::format("%s%s", killed_player->name.c_str(), mui_msg);
    }
    else if (killed_player == rf::local_player) {
        color_id = rf::ChatMsgColor::white_white;
        if (killer_player == killed_player) {
            mui_msg = null_to_empty(rf::strings::you_killed_yourself);
            msg = rf::String::format("%s", mui_msg);
        }
        else if (killer_entity && killer_entity->ai.current_primary_weapon == rf::riot_stick_weapon_type) {
            mui_msg = null_to_empty(rf::strings::you_just_got_beat_down_by);
            msg = rf::String::format("%s%s!", mui_msg, killer_player->name.c_str());
        }
        else {
            mui_msg = null_to_empty(rf::strings::you_were_killed_by);

            const char* weapon_name = nullptr;
            int killer_weapon_cls_id = killer_entity ? killer_entity->ai.current_primary_weapon : -1;
            if (killer_weapon_cls_id >= 0 && killer_weapon_cls_id < 64) {
                auto& weapon_cls = rf::weapon_types[killer_weapon_cls_id];
                weapon_name = weapon_cls.display_name.c_str();
            }

            auto killer_name = killer_player->name.c_str();
            if (weapon_name)
                msg = rf::String::format("%s%s (%s)!", mui_msg, killer_name, weapon_name);
            else
                msg = rf::String::format("%s%s!", mui_msg, killer_name);
        }
    }
    else if (killer_player == rf::local_player) {
        color_id = rf::ChatMsgColor::white_white;
        mui_msg = null_to_empty(rf::strings::you_killed);
        msg = rf::String::format("%s%s!", mui_msg, killed_player->name.c_str());
    }
    else {
        color_id = rf::ChatMsgColor::default_;
        if (killer_player == killed_player) {
            if (rf::multi_entity_is_female(killed_player->settings.multi_character))
                mui_msg = null_to_empty(rf::strings::was_killed_by_her_own_hand);
            else
                mui_msg = null_to_empty(rf::strings::was_killed_by_his_own_hand);
            msg = rf::String::format("%s%s", killed_player->name.c_str(), mui_msg);
        }
        else {
            if (killer_entity && killer_entity->ai.current_primary_weapon == rf::riot_stick_weapon_type)
                mui_msg = null_to_empty(rf::strings::got_beat_down_by);
            else
                mui_msg = null_to_empty(rf::strings::was_killed_by);
            msg = rf::String::format("%s%s%s", killed_player->name.c_str(), mui_msg, killer_player->name.c_str());
        }
    }

    rf::String prefix;
    rf::multi_chat_print(msg, color_id, prefix);

    auto killed_stats = reinterpret_cast<PlayerStatsNew*>(killed_player->stats);
    ++killed_stats->num_deaths;

    if (killer_player) {
        auto killer_stats = reinterpret_cast<PlayerStatsNew*>(killer_player->stats);
        if (killer_player != killed_player) {
            rf::player_add_score(killer_player, 1);
            ++killer_stats->num_kills;
        }
        else
            rf::player_add_score(killer_player, -1);
    }
}

FunHook<void(rf::Entity*)> entity_on_death_hook{
    0x0041FDC0,
    [](rf::Entity* entity) {
        // Reset fpgun animation when player dies
        if (rf::local_player && entity->handle == rf::local_player->entity_handle && rf::local_player->weapon_mesh_handle) {
            player_fpgun_on_player_death(rf::local_player);
        }
        entity_on_death_hook.call_target(entity);
    },
};

void multi_kill_do_patch()
{
    // Player kill handling
    using namespace asm_regs;
    AsmWriter(0x00420703)
        .push(ebx)
        .push(edi)
        .call(on_player_kill)
        .add(esp, 8)
        .jmp(0x00420B03);

    // Change player stats structure
    write_mem<i8>(0x004A33B5 + 1, sizeof(PlayerStatsNew));
    multi_level_init_hook.install();

    // Reset fpgun animation when player dies
    entity_on_death_hook.install();
}
