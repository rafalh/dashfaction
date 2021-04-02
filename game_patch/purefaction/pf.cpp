
#include <common/config/BuildConfig.h>
#include "pf.h"
#include "pf_packets.h"
#include "pf_secret.h"
#include "../rf/multi.h"
#include "../multi/multi.h"
#include <common/utils/list-utils.h>
#include <cstddef>
#include <sstream>
#include <xlog/xlog.h>

void process_pf_player_announce_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NetAddr& addr)
{
    // Client-side packet
    if (rf::is_server)
        return;

    if (len < sizeof(pf_player_announce_packet))
        return;

    auto& in_packet = *reinterpret_cast<const pf_player_announce_packet*>(data);
    if (in_packet.version != 2)
        return;

    xlog::trace("PF player_announce packet: player %u is_pure %d", in_packet.player_id, in_packet.is_pure);

    if (in_packet.player_id == rf::local_player->net_data->player_id) {
        static const char* pf_verification_status_names[] = { "none", "blue", "gold", "red" };
        auto pf_verification_status =
            in_packet.is_pure < std::size(pf_verification_status_names)
            ? pf_verification_status_names[in_packet.is_pure] : "unknown";
        xlog::info("PF Verification Status: %s (%u)", pf_verification_status, in_packet.is_pure);
    }
}

void process_pf_player_stats_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NetAddr& addr)
{
    // Client-side packet
    if (rf::is_server)
        return;

    if (len < sizeof(pf_player_stats_packet))
        return;

    auto& in_packet = *reinterpret_cast<const pf_player_stats_packet*>(data);

    if (in_packet.version != 2)
        return;

    xlog::trace("PF player_stats packet (count %d)", in_packet.player_count);

    auto players_stats = reinterpret_cast<const pf_player_stats_packet::player_stats*>(&in_packet + 1);
    for (int i = 0; i < in_packet.player_count; ++i) {
        auto& player_data = players_stats[i];
        auto player = rf::multi_find_player_by_id(player_data.player_id);
        if (player) {
            auto& stats = *reinterpret_cast<PlayerStatsNew*>(player->stats);
            stats.num_kills = player_data.kills;
            stats.num_deaths = player_data.deaths;
        }
        else {
            xlog::warn("PF player_stats packet: player %u not found", player_data.player_id);
        }
    }
}

void process_pf_players_request_packet([[ maybe_unused ]] const void* data, [[ maybe_unused ]] size_t len, const rf::NetAddr& addr)
{
    xlog::trace("PF players_request packet");

    // Server-side packet
    if (!rf::is_server) {
        return;
    }

    std::stringstream ss;
    auto player_list = SinglyLinkedList(rf::player_list);
    for (auto& player : player_list) {
        ss.write(player.name.c_str(), player.name.size() + 1);
    }
    auto players_str = ss.str();

    auto packet_buf = std::make_unique<std::byte[]>(sizeof(pf_players_packet) + players_str.size());
    auto& response = *reinterpret_cast<pf_players_packet*>(packet_buf.get());
    response.hdr.type = static_cast<uint8_t>(pf_packet_type::players);
    response.hdr.size = static_cast<uint16_t>(sizeof(pf_players_packet) + players_str.size() - sizeof(rf_packet_header));
    response.version = 1;
    response.show_ip = 0;
    std::copy(players_str.begin(), players_str.end(), reinterpret_cast<char*>(packet_buf.get() + sizeof(pf_players_packet)));

    rf::net_send(addr, &response, sizeof(response.hdr) + response.hdr.size);
}

void process_pf_packet(const void* data, int len, const rf::NetAddr& addr, [[maybe_unused]] const rf::Player* player)
{
    if (len < static_cast<int>(sizeof(rf_packet_header)))
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
            process_pf_player_announce_packet(data, len, addr);
            break;

        case pf_packet_type::player_stats:
            process_pf_player_stats_packet(data, len, addr);
            break;

        case pf_packet_type::players_request:
            process_pf_players_request_packet(data, len, addr);
            break;

        default:
            // ignore
            break;
    }
}
