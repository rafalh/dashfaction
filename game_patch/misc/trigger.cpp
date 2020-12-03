#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <common/rfproto.h>
#include <xlog/xlog.h>
#include "../rf/common.h"
#include "../rf/player.h"
#include "../rf/trigger.h"
#include "../rf/multi.h"

constexpr char TRIGGER_PF_FLAGS_PREFIX = '\xAB';
constexpr uint8_t TRIGGER_CLIENT_SIDE = 0x2;
constexpr uint8_t TRIGGER_SOLO = 0x4;
constexpr uint8_t TRIGGER_TELEPORT = 0x8;

rf::Player* g_trigger_solo_player = nullptr;
std::vector<int> g_triggers_uids_for_late_joiners;

void SendTriggerActivatePacket(rf::Player* player, int trigger_uid, int32_t entity_handle)
{
    RF_TriggerActivatePacket packet;
    packet.header.type = RF_GPT_TRIGGER_ACTIVATE;
    packet.header.size = sizeof(packet) - sizeof(packet.header);
    packet.uid = trigger_uid;
    packet.entity_handle = entity_handle;
    rf::multi_io_send_reliable(player, reinterpret_cast<uint8_t*>(&packet), sizeof(packet), 0);
}

FunHook<void(int, int)> send_trigger_activate_packet_to_all_players_hook{
    0x00483190,
    [](int trigger_uid, int entity_handle) {
        if (g_trigger_solo_player)
            SendTriggerActivatePacket(g_trigger_solo_player, trigger_uid, entity_handle);
        else
            send_trigger_activate_packet_to_all_players_hook.call_target(trigger_uid, entity_handle);
    },
};

FunHook<void(rf::Trigger*, int32_t, bool)> trigger_activate_hook{
    0x004C0220,
    [](rf::Trigger* trigger, int32_t h_entity, bool skip_movers) {
        // Check team
        auto player = rf::player_from_entity_handle(h_entity);
        auto trigger_name = trigger->name.c_str();
        if (player && trigger->team != -1 && trigger->team != player->team) {
            // rf::console_printf("Trigger team does not match: %d vs %d (%s)", trigger->team, Player->blue_team,
            // trigger_name);
            return;
        }

        // Check if this is Solo or Teleport trigger (REDPF feature)
        uint8_t ext_flags = trigger_name[0] == TRIGGER_PF_FLAGS_PREFIX ? trigger_name[1] : 0;
        bool is_solo_trigger = (ext_flags & (TRIGGER_SOLO | TRIGGER_TELEPORT)) != 0;
        if (rf::is_multi && rf::is_server && is_solo_trigger && player) {
            // rf::console_printf("Solo/Teleport trigger activated %s", trigger_name);
            if (player != rf::local_player) {
                SendTriggerActivatePacket(player, trigger->uid, h_entity);
                return;
            }
            else {
                g_trigger_solo_player = player;
            }
        }

        // Resets times set to 1 enabled late joiners support in Pure Faction
        if (rf::is_multi && rf::is_server && trigger->max_count == 1) {
            g_triggers_uids_for_late_joiners.push_back(trigger->uid);
        }

        // Normal activation
        // rf::console_printf("trigger normal activation %s %d", trigger_name, ext_flags);
        trigger_activate_hook.call_target(trigger, h_entity, skip_movers);
        g_trigger_solo_player = nullptr;
    },
};

CodeInjection trigger_check_activation_patch{
    0x004BFC7D,
    [](auto& regs) {
        auto trigger = reinterpret_cast<rf::Trigger*>(regs.eax);
        auto trigger_name = trigger->name.c_str();
        uint8_t ext_flags = trigger_name[0] == TRIGGER_PF_FLAGS_PREFIX ? trigger_name[1] : 0;
        bool is_client_side = (ext_flags & TRIGGER_CLIENT_SIDE) != 0;
        if (is_client_side)
            regs.eip = 0x004BFCDB;
    },
};

void ActivateTriggersForLateJoiner(rf::Player* player)
{
    for (auto trigger_uid : g_triggers_uids_for_late_joiners) {
        xlog::debug("Activating trigger %d for late joiner %s", trigger_uid, player->name.c_str());
        SendTriggerActivatePacket(player, trigger_uid, -1);
    }
}

void ClearTriggersForLateJoiners()
{
    g_triggers_uids_for_late_joiners.clear();
}

CodeInjection send_state_info_injection{
    0x0048186F,
    [](auto& regs) {
        auto player = reinterpret_cast<rf::Player*>(regs.edi);
        ActivateTriggersForLateJoiner(player);
    },
};

void ApplyTriggerPatches()
{
    // Solo/Teleport triggers handling + filtering by team ID
    trigger_activate_hook.install();
    send_trigger_activate_packet_to_all_players_hook.install();

    // Client-side trigger flag handling
    trigger_check_activation_patch.install();

    // Send trigger_activate packets for late joiners
    send_state_info_injection.install();
}
