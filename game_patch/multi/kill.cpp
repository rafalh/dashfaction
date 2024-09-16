#include <common/utils/list-utils.h>
#include <patch_common/FunHook.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include "multi.h"
#include "../os/console.h"
#include "../rf/player/player.h"
#include "../rf/entity.h"
#include "../rf/localize.h"
#include "../rf/multi.h"
#include "../rf/weapon.h"
#include "../hud/multi_spectate.h"
#include "server_internal.h"

bool kill_messages = true;

void player_fpgun_on_player_death(rf::Player* pp);

void multi_kill_init_player(rf::Player* player)
{
    auto* stats = static_cast<PlayerStatsNew*>(player->stats);
    stats->clear();
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

void print_kill_message(rf::Player* killed_player, rf::Player* killer_player)
{
    rf::String msg;
    const char* mui_msg;
    rf::ChatMsgColor color_id;

    rf::Entity* killer_entity = killer_player ? rf::entity_from_handle(killer_player->entity_handle) : nullptr;

    if (!killer_player) {
        color_id = rf::ChatMsgColor::default_;
        mui_msg = null_to_empty(rf::strings::was_killed_mysteriously);
        msg = rf::String::format("{}{}", killed_player->name, mui_msg);
    }
    else if (killed_player == rf::local_player) {
        color_id = rf::ChatMsgColor::white_white;
        if (killer_player == killed_player) {
            mui_msg = null_to_empty(rf::strings::you_killed_yourself);
            msg = rf::String::format("{}", mui_msg);
        }
        else if (killer_entity && killer_entity->ai.current_primary_weapon == rf::riot_stick_weapon_type) {
            mui_msg = null_to_empty(rf::strings::you_just_got_beat_down_by);
            msg = rf::String::format("{}{}!", mui_msg, killer_player->name);
        }
        else {
            mui_msg = null_to_empty(rf::strings::you_were_killed_by);

            auto& killer_name = killer_player->name;
            int killer_weapon_cls_id = killer_entity ? killer_entity->ai.current_primary_weapon : -1;
            if (killer_weapon_cls_id >= 0 && killer_weapon_cls_id < 64) {
                auto& weapon_cls = rf::weapon_types[killer_weapon_cls_id];
                auto& weapon_name = weapon_cls.display_name;
                msg = rf::String::format("{}{} ({})!", mui_msg, killer_name, weapon_name);
            }
            else {
                msg = rf::String::format("{}{}!", mui_msg, killer_name);
            }
        }
    }
    else if (killer_player == rf::local_player) {
        color_id = rf::ChatMsgColor::white_white;
        mui_msg = null_to_empty(rf::strings::you_killed);
        msg = rf::String::format("{}{}!", mui_msg, killed_player->name);
    }
    else {
        color_id = rf::ChatMsgColor::default_;
        if (killer_player == killed_player) {
            if (rf::multi_entity_is_female(killed_player->settings.multi_character))
                mui_msg = null_to_empty(rf::strings::was_killed_by_her_own_hand);
            else
                mui_msg = null_to_empty(rf::strings::was_killed_by_his_own_hand);
            msg = rf::String::format("{}{}", killed_player->name, mui_msg);
        }
        else {
            if (killer_entity && killer_entity->ai.current_primary_weapon == rf::riot_stick_weapon_type)
                mui_msg = null_to_empty(rf::strings::got_beat_down_by);
            else
                mui_msg = null_to_empty(rf::strings::was_killed_by);
            msg = rf::String::format("{}{}{}", killed_player->name, mui_msg, killer_player->name);
        }
    }

    rf::String prefix;
    rf::multi_chat_print(msg, color_id, prefix);
}

void multi_apply_kill_reward(rf::Player* player)
{
    rf::Entity* ep = rf::entity_from_handle(player->entity_handle);
    if (!ep) {
        return;
    }

    const auto& conf = server_get_df_config();

    float max_life = conf.kill_reward_health_super ? 200.0f : ep->info->max_life;
    float max_armor = conf.kill_reward_armor_super ? 200.0f : ep->info->max_armor;

    if (conf.kill_reward_health > 0.0f) {
        ep->life = std::min(ep->life + conf.kill_reward_health, max_life);
    }
    if (conf.kill_reward_armor > 0.0f) {
        ep->armor = std::min(ep->armor + conf.kill_reward_armor, max_armor);
    }
    if (conf.kill_reward_effective_health > 0.0f) {
        float life_to_add = std::min(conf.kill_reward_effective_health, max_life - ep->life);
        float armor_to_add = std::min((conf.kill_reward_effective_health - life_to_add) / 2, max_armor - ep->armor);
        ep->life += life_to_add;
        ep->armor += armor_to_add;
    }
}

void on_player_kill(rf::Player* killed_player, rf::Player* killer_player)
{
    if (kill_messages) {
        print_kill_message(killed_player, killer_player);
    }

    auto* killed_stats = static_cast<PlayerStatsNew*>(killed_player->stats);
    killed_stats->inc_deaths();

    if (killer_player) {
        auto* killer_stats = static_cast<PlayerStatsNew*>(killer_player->stats);
        if (killer_player != killed_player) {
            rf::player_add_score(killer_player, 1);
            killer_stats->inc_kills();
        }
        else {
            rf::player_add_score(killer_player, -1);
        }

        multi_apply_kill_reward(killer_player);

        multi_spectate_on_player_kill(killed_player, killer_player);
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

ConsoleCommand2 kill_messages_cmd{
    "kill_messages",
    []() {
        kill_messages = !kill_messages;
    },
    "Toggles printing of kill messages in the chatbox and the game console",
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

    // Allow disabling kill messages
    kill_messages_cmd.register_cmd();
}
