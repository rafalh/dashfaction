#include <cstddef>
#include <cstring>
#include <sstream>
#include <cassert>
#include <common/config/BuildConfig.h>
#include <common/utils/list-utils.h>
#include <xlog/xlog.h>
#include "../rf/multi.h"
#include "../multi/multi.h"
#include "pf.h"
#include "pf_packets.h"
#include "pf_secret.h"

static void send_pf_announce_player_packet(rf::Player* player, pf_pure_status pure_status)
{
    // Send: server -> client
    assert(rf::is_server);

    pf_player_announce_packet announce_packet;
    announce_packet.hdr.type = static_cast<uint8_t>(pf_packet_type::announce_player);
    announce_packet.hdr.size = sizeof(announce_packet); // should not include header size (PF bug)
    announce_packet.version = pf_announce_player_packet_version;
    announce_packet.player_id = player->net_data->player_id;
    announce_packet.is_pure = static_cast<uint8_t>(pure_status);

    auto player_list = SinglyLinkedList(rf::player_list);
    for (auto& other_player : player_list) {
        rf::net_send(other_player.net_data->addr, &announce_packet, sizeof(announce_packet));
    }
}

static void process_pf_player_announce_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NetAddr& addr)
{
    // Receive: client <- server
    if (rf::is_server) {
        return;
    }

    pf_player_announce_packet announce_packet;
    if (len < sizeof(announce_packet)) {
        xlog::trace("Invalid length in PF player_announce packet");
        return;
    }

    std::memcpy(&announce_packet, data, sizeof(announce_packet));

    if (announce_packet.version != pf_announce_player_packet_version) {
        xlog::trace("Invalid version in PF player_announce packet");
        return;
    }

    xlog::trace("PF player_announce packet: player %u is_pure %d", announce_packet.player_id, announce_packet.is_pure);

    if (announce_packet.player_id == rf::local_player->net_data->player_id) {
        static const char* pf_verification_status_names[] = { "none", "blue", "gold", "red" };
        auto pf_verification_status =
            announce_packet.is_pure < std::size(pf_verification_status_names)
            ? pf_verification_status_names[announce_packet.is_pure] : "unknown";
        xlog::info("PF Verification Status: %s (%u)", pf_verification_status, announce_packet.is_pure);
    }
}

static void send_pf_player_stats_packet(rf::Player* player)
{
    // Send: server -> client
    assert(rf::is_server);

    std::byte packet_buf[512];
    pf_player_stats_packet stats_packet;
    stats_packet.hdr.type = static_cast<uint8_t>(pf_packet_type::player_stats);
    stats_packet.hdr.size = sizeof(stats_packet); // should not include header size (PF bug)
    stats_packet.version = pf_player_stats_packet_version;
    stats_packet.player_count = 0;

    std::stringstream ss;
    auto player_list = SinglyLinkedList{rf::player_list};
    for (auto& current_player : player_list) {
        auto& player_stats = *static_cast<PlayerStatsNew*>(current_player.stats);
        pf_player_stats_packet::player_stats out_stats;
        out_stats.player_id = current_player.net_data->player_id;
        out_stats.is_pure = 0;
        out_stats.accuracy = 0;
        out_stats.streak_max = 0;
        out_stats.streak_current = 0;
        out_stats.kills = player_stats.num_kills;
        out_stats.deaths = player_stats.num_deaths;
        out_stats.team_kills = 0;
        ++stats_packet.player_count;
        stats_packet.hdr.size += sizeof(out_stats);
        ss.write(reinterpret_cast<char*>(&out_stats), sizeof(out_stats));
    }

    auto stats_str = ss.str();
    int packet_len = sizeof(stats_packet) + stats_str.size();
    if (static_cast<size_t>(packet_len) > sizeof(packet_buf)) {
        // One packet should be enough for 32 players (13 bytes * 32 players = 416 bytes)
        // Handling more data would require sending multiple packets to avoid building one big UDP datagram
        // that would cross 512 bytes limit that is usually used as maximal datagram size
        xlog::error("PF player_stats packet is too big: %d", packet_len);
        return;
    }

    std::memcpy(packet_buf, &stats_packet, sizeof(stats_packet));
    std::memcpy(packet_buf + sizeof(stats_packet), stats_str.data(), stats_str.size());
    rf::net_send(player->net_data->addr, packet_buf, packet_len);
}

static void process_pf_player_stats_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NetAddr& addr)
{
    // Receive: client <- server
    if (rf::is_server) {
        return;
    }

    pf_player_stats_packet stats_packet;
    if (len < sizeof(stats_packet)) {
        xlog::trace("Invalid length in PF player_stats packet");
        return;
    }

    std::memcpy(&stats_packet, data, sizeof(stats_packet));

    if (stats_packet.version != pf_player_stats_packet_version) {
        xlog::trace("Invalid version in PF player_stats packet");
        return;
    }

    if (len < sizeof(stats_packet) + stats_packet.player_count * sizeof(pf_player_stats_packet::player_stats)) {
        xlog::trace("Invalid length in pf_player_stats packet");
        return;
    }

    xlog::trace("PF player_stats packet (count %d)", stats_packet.player_count);

    const std::byte* player_stats_ptr = static_cast<const std::byte*>(data) + sizeof(stats_packet);
    for (int i = 0; i < stats_packet.player_count; ++i) {
        pf_player_stats_packet::player_stats in_stats;
        std::memcpy(&in_stats, player_stats_ptr, sizeof(in_stats));
        player_stats_ptr += sizeof(in_stats);
        auto player = rf::multi_find_player_by_id(in_stats.player_id);
        if (player) {
            auto& stats = *static_cast<PlayerStatsNew*>(player->stats);
            stats.num_kills = in_stats.kills;
            stats.num_deaths = in_stats.deaths;
        }
        else {
            xlog::warn("PF player_stats packet: player %u not found", in_stats.player_id);
        }
    }
}

static void process_pf_players_request_packet([[ maybe_unused ]] const void* data, [[ maybe_unused ]] size_t len, const rf::NetAddr& addr)
{
    xlog::trace("PF players_request packet");

    // Receive: server <- client (can be non-joined)
    if (!rf::is_server) {
        return;
    }

    std::stringstream ss;
    auto player_list = SinglyLinkedList(rf::player_list);
    for (auto& player : player_list) {
        ss.write(player.name.c_str(), player.name.size() + 1);
    }
    auto players_str = ss.str();

    pf_players_packet players_packet;
    players_packet.hdr.type = static_cast<uint8_t>(pf_packet_type::players);
    players_packet.hdr.size = static_cast<uint16_t>(sizeof(pf_players_packet) + players_str.size() - sizeof(rf_packet_header));
    players_packet.version = 1;
    players_packet.show_ip = 0;

    auto response_size = sizeof(players_packet) + players_str.size();
    auto response_buf = std::make_unique<std::byte[]>(response_size);
    std::memcpy(response_buf.get(), &players_packet, sizeof(players_packet));
    std::memcpy(response_buf.get() + sizeof(players_packet), players_str.data(), players_str.size());
    rf::net_send(addr, response_buf.get(), response_size);
}

void process_pf_packet(const void* data, int len, const rf::NetAddr& addr, rf::Player* player)
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

        case pf_packet_type::client_hash:
            process_pf_client_hash_packet(data, len, addr, player);
            break;

        case pf_packet_type::request_cheat_check:
            process_pf_request_cheat_check_packet(data, len, addr);
            break;

        case pf_packet_type::client_cheat_check:
            process_pf_client_cheat_check_packet(data, len, player);
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

void pf_player_init([[ maybe_unused ]] rf::Player* player)
{
    assert(rf::is_server);
    //send_pf_server_hash_packet(player);
}

void pf_player_level_load(rf::Player* player)
{
    assert(rf::is_server);
    send_pf_server_hash_packet(player);
}

void pf_player_verified(rf::Player* player, pf_pure_status pure_status)
{
    assert(rf::is_server);
    send_pf_announce_player_packet(player, pure_status);
    send_pf_player_stats_packet(player);
}
