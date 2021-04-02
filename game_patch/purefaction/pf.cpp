
#include <common/config/BuildConfig.h>
#include "pf.h"
#include "pf_packets.h"
#include "pf_secret.h"
#include "../rf/multi.h"
#include "../multi/multi.h"
#include <common/utils/list-utils.h>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <xlog/xlog.h>

void process_pf_player_announce_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NetAddr& addr)
{
    // Client-side packet
    if (rf::is_server) {
        return;
    }

    pf_player_announce_packet in_packet;
    if (len < sizeof(in_packet)) {
        xlog::trace("Invalid length in PF player_announce packet");
        return;
    }

    std::memcpy(&in_packet, data, sizeof(in_packet));

    if (in_packet.version != 2) {
        xlog::trace("Invalid version in PF player_announce packet");
        return;
    }

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
    if (rf::is_server) {
        return;
    }

    pf_player_stats_packet in_packet;
    if (len < sizeof(in_packet)) {
        xlog::trace("Invalid length in PF player_stats packet");
        return;
    }

    std::memcpy(&in_packet, data, sizeof(in_packet));

    if (in_packet.version != 2) {
        xlog::trace("Invalid version in PF player_stats packet");
        return;
    }

    if (len < sizeof(in_packet) + in_packet.player_count * sizeof(pf_player_stats_packet::player_stats)) {
        xlog::trace("Invalid length in pf_player_stats packet");
        return;
    }

    xlog::trace("PF player_stats packet (count %d)", in_packet.player_count);

    auto player_stats_ptr = reinterpret_cast<const std::byte*>(data) + sizeof(in_packet);
    for (int i = 0; i < in_packet.player_count; ++i) {
        pf_player_stats_packet::player_stats in_stats;
        std::memcpy(&in_stats, player_stats_ptr, sizeof(in_stats));
        player_stats_ptr += sizeof(in_stats);
        auto player = rf::multi_find_player_by_id(in_stats.player_id);
        if (player) {
            auto& stats = *reinterpret_cast<PlayerStatsNew*>(player->stats);
            stats.num_kills = in_stats.kills;
            stats.num_deaths = in_stats.deaths;
        }
        else {
            xlog::warn("PF player_stats packet: player %u not found", in_stats.player_id);
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

    pf_players_packet response_fixed_part;
    response_fixed_part.hdr.type = static_cast<uint8_t>(pf_packet_type::players);
    response_fixed_part.hdr.size = static_cast<uint16_t>(sizeof(pf_players_packet) + players_str.size() - sizeof(rf_packet_header));
    response_fixed_part.version = 1;
    response_fixed_part.show_ip = 0;

    auto response_size = sizeof(response_fixed_part) + players_str.size();
    auto response_buf = std::make_unique<std::byte[]>(response_size);
    std::memcpy(response_buf.get(), &response_fixed_part, sizeof(response_fixed_part));
    std::memcpy(response_buf.get() + sizeof(response_fixed_part), players_str.data(), players_str.size());
    rf::net_send(addr, response_buf.get(), response_size);
}

void process_pf_packet(const void* data, int len, const rf::NetAddr& addr, [[maybe_unused]] const rf::Player* player)
{
    rf_packet_header header;
    if (len < static_cast<int>(sizeof(header))) {
        return;
    }

    std::memcpy(&header, data, sizeof(header));
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
