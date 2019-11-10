#include "../stdafx.h"
#include "pf.h"
#include "pf_packets.h"
#include "pf_secret.h"
#include "../kill.h"

void process_player_announce_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NwAddr& addr)
{
    if (len < sizeof(pf_player_announce_packet))
        return;

    auto& in_packet = *reinterpret_cast<const pf_player_announce_packet*>(data);
    if (in_packet.version != 2)
        return;

    TRACE("PF player_announce packet: player %u is_pure %d", in_packet.player_id, in_packet.is_pure);

    if (in_packet.player_id == rf::local_player->nw_data->player_id) {
        static const char* pf_verification_status_names[] = { "none", "blue", "gold", "red" };
        auto pf_verification_status =
            in_packet.is_pure < std::size(pf_verification_status_names)
            ? pf_verification_status_names[in_packet.is_pure] : "unknown";
        INFO("PF Verification Status: %s (%u)", pf_verification_status, in_packet.is_pure);
    }
}

void process_player_stats_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NwAddr& addr)
{
    if (len < sizeof(pf_player_announce_packet))
        return;

    auto& in_packet = *reinterpret_cast<const pf_player_stats_packet*>(data);

    if (in_packet.version != 2)
        return;

    TRACE("PF player_stats packet (count %d)", in_packet.player_count);

    auto players_stats = reinterpret_cast<const pf_player_stats_packet::player_stats*>(&in_packet + 1);
    for (int i = 0; i < in_packet.player_count; ++i) {
        auto& player_data = players_stats[i];
        auto player = rf::GetPlayerById(player_data.player_id);
        if (player) {
            auto& stats = *reinterpret_cast<PlayerStatsNew*>(player->stats);
            stats.num_kills = player_data.kills;
            stats.num_deaths = player_data.deaths;
        }
        else {
            WARN("PF player_stats packet: player %u not found", player_data.player_id);
        }
    }
}

void ProcessPfPacket(const void* data, size_t len, const rf::NwAddr& addr, [[maybe_unused]] const rf::Player* player)
{
    if (len < sizeof(rf_packet_header))
        return;

    auto& header = *reinterpret_cast<const rf_packet_header*>(data);
    auto packet_type = static_cast<pf_packet_type>(header.type);

    switch (packet_type)
    {
        case pf_packet_type::server_hash:
            process_pf_server_hash_packet(data, len, addr);
            break;

        case pf_packet_type::request_cheat_check:
            process_pf_request_cheat_check_packet(data, len, addr);
            break;

        case pf_packet_type::announce_player:
            process_player_announce_packet(data, len, addr);
            break;

        case pf_packet_type::player_stats:
            process_player_stats_packet(data, len, addr);
            break;

        default:
            // ignore
            break;
    }
}
