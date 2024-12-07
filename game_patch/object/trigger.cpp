#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <common/rfproto.h>
#include <xlog/xlog.h>
#include "../rf/player/player.h"
#include "../rf/trigger.h"
#include "../rf/multi.h"

constexpr char TRIGGER_PF_FLAGS_PREFIX = '\xAB';
constexpr uint8_t TRIGGER_CLIENT_SIDE = 0x2;
constexpr uint8_t TRIGGER_SOLO = 0x4;
constexpr uint8_t TRIGGER_TELEPORT = 0x8;

rf::Player* g_trigger_solo_player = nullptr;
std::vector<int> g_triggers_uids_for_late_joiners;

void send_trigger_activate_packet(rf::Player* player, int trigger_uid, int32_t entity_handle)
{
    RF_TriggerActivatePacket packet;
    packet.header.type = RF_GPT_TRIGGER_ACTIVATE;
    packet.header.size = sizeof(packet) - sizeof(packet.header);
    packet.uid = trigger_uid;
    packet.entity_handle = entity_handle;
    rf::multi_io_send_reliable(player, &packet, sizeof(packet), 0);
}

FunHook<void(int, int)> send_trigger_activate_packet_to_all_players_hook{
    0x00483190,
    [](int trigger_uid, int entity_handle) {
        if (g_trigger_solo_player)
            send_trigger_activate_packet(g_trigger_solo_player, trigger_uid, entity_handle);
        else
            send_trigger_activate_packet_to_all_players_hook.call_target(trigger_uid, entity_handle);
    },
};

FunHook<void(rf::Trigger*, int32_t, bool)> trigger_activate_hook{
    0x004C0220,
    [](rf::Trigger* trigger, int32_t h_entity, bool skip_movers) {
        // Check team
        rf::Player* player = rf::player_from_entity_handle(h_entity);
        const char* trigger_name = trigger->name.c_str();
        if (player && trigger->trigger_team != 0xFF && trigger->trigger_team != player->team) {
            // rf::console::print("Trigger team does not match: {} vs {} ({})", trigger->team, Player->blue_team,
            // trigger_name);
            return;
        }

        // Check if this is Solo or Teleport trigger (REDPF feature)
        uint8_t ext_flags = trigger_name[0] == TRIGGER_PF_FLAGS_PREFIX ? trigger_name[1] : 0;
        bool is_solo_trigger = (ext_flags & (TRIGGER_SOLO | TRIGGER_TELEPORT)) != 0;
        if (rf::is_multi && rf::is_server && is_solo_trigger && player) {
            // rf::console::print("Solo/Teleport trigger activated {}", trigger_name);
            if (player != rf::local_player) {
                send_trigger_activate_packet(player, trigger->uid, h_entity);
                return;
            }
            g_trigger_solo_player = player;
        }

        // Resets times set to 1 enabled late joiners support in Pure Faction
        if (rf::is_multi && rf::is_server && trigger->max_count == 1) {
            g_triggers_uids_for_late_joiners.push_back(trigger->uid);
        }

        // Normal activation
        // rf::console::print("trigger normal activation {} {}", trigger_name, ext_flags);
        trigger_activate_hook.call_target(trigger, h_entity, skip_movers);
        g_trigger_solo_player = nullptr;
    },
};

CodeInjection trigger_check_activation_patch{
    0x004BFC7D,
    [](auto& regs) {
        rf::Trigger* trigger = regs.eax;
        const char* trigger_name = trigger->name.c_str();
        uint8_t ext_flags = trigger_name[0] == TRIGGER_PF_FLAGS_PREFIX ? trigger_name[1] : 0;
        bool is_client_side = (ext_flags & TRIGGER_CLIENT_SIDE) != 0;
        if (is_client_side)
            regs.eip = 0x004BFCDB;
    },
};

void trigger_send_state_info(rf::Player* player)
{
    for (auto trigger_uid : g_triggers_uids_for_late_joiners) {
        xlog::debug("Activating trigger {} for late joiner {}", trigger_uid, player->name);
        send_trigger_activate_packet(player, trigger_uid, -1);
    }
}

FunHook<void()> trigger_level_init{
    0x004BF730,
    []() {
        trigger_level_init.call_target();
        g_triggers_uids_for_late_joiners.clear();
    },
};

void trigger_apply_patches()
{
    // Solo/Teleport triggers handling + filtering by team ID
    trigger_activate_hook.install();
    send_trigger_activate_packet_to_all_players_hook.install();

    // Client-side trigger flag handling
    trigger_check_activation_patch.install();

    // level init hook
    trigger_level_init.install();
}
