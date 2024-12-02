#include <cstddef>
#include <cstring>
#include <cassert>
#include <array>
#include <common/config/BuildConfig.h>
#include <common/utils/list-utils.h>
#include <xlog/xlog.h>
#include "../rf/multi.h"
#include "../multi/multi.h"
#include "pf.h"
#include "pf_packets.h"
#include "pf_ac.h"
#include "../misc/player.h"

void pf_send_reliable_packet(rf::Player* player, const void* data, int len)
{
#if 1 // reliable
    // PF improperly handles custom packets if they are not the first ones in a reliable packets container so flush
    // buffer before sending
    rf::multi_io_send_buffered_reliable_packets(player);
    // Buffer data in a reliable packet
    rf::multi_io_send_reliable(player, data, len, 0);
    // RF stops reading reliable packets after encountering the first unknown type so flush buffer after sending
    rf::multi_io_send_buffered_reliable_packets(player);
#else // unreliable
    rf::net_send(player->net_data->addr, data, len);
#endif
}

static void send_pf_announce_player_packet(rf::Player* player, pf_pure_status pure_status)
{
    // Send: server -> client
    assert(rf::is_server);

    pf_player_announce_packet announce_packet{};
    announce_packet.hdr.type = static_cast<uint8_t>(pf_packet_type::announce_player);
    announce_packet.hdr.size = sizeof(announce_packet) - sizeof(announce_packet.hdr);
    announce_packet.version = pf_announce_player_packet_version;
    announce_packet.player_id = player->net_data->player_id;
    announce_packet.is_pure = static_cast<uint8_t>(
        get_player_additional_data(player).is_browser ? pf_pure_status::rfsb : pure_status
    );

    auto player_list = SinglyLinkedList(rf::player_list);
    for (auto& other_player : player_list) {
        pf_send_reliable_packet(&other_player, &announce_packet, sizeof(announce_packet));
    }
}

static void process_pf_player_announce_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NetAddr& addr)
{
    // Receive: client <- server
    if (rf::is_server) {
        return;
    }

    pf_player_announce_packet announce_packet{};
    if (len < sizeof(announce_packet)) {
        xlog::trace("Invalid length in PF player_announce packet");
        return;
    }

    std::memcpy(&announce_packet, data, sizeof(announce_packet));

    if (announce_packet.version != pf_announce_player_packet_version) {
        xlog::trace("Invalid version in PF player_announce packet");
        return;
    }

    xlog::trace("PF player_announce packet: player {} is_pure {}", announce_packet.player_id, announce_packet.is_pure);
    
    if (rf::Player* player = rf::multi_find_player_by_id(announce_packet.player_id); player
        && announce_packet.is_pure <= static_cast<uint8_t>(pf_pure_status::_last_variant)) {
        get_player_additional_data(player).received_ac_status
            = std::optional{static_cast<pf_pure_status>(announce_packet.is_pure)};
    }

    if (announce_packet.player_id == rf::local_player->net_data->player_id) {
        static constinit const std::array<std::string_view, 6> pf_verification_status_names{
            {"none", "blue", "gold", "fail", "old_blue", "rfsb"}
        };
        const std::string_view pf_verification_status
            = announce_packet.is_pure < std::size(pf_verification_status_names)
            ? pf_verification_status_names[announce_packet.is_pure]
            : "unknown";
        xlog::info("PF Verification Status: {} ({})", pf_verification_status, announce_packet.is_pure);
    }
}

void send_pf_player_stats_packet(rf::Player* player)
{
    // Send: server -> client
    assert(rf::is_server);

    std::byte packet_buf[rf::max_packet_size];
    pf_player_stats_packet stats_packet{};
    stats_packet.hdr.type = static_cast<uint8_t>(pf_packet_type::player_stats);
    stats_packet.hdr.size = sizeof(stats_packet) - sizeof(stats_packet.hdr);
    stats_packet.version = pf_player_stats_packet_version;
    stats_packet.player_count = 0;

    size_t packet_len = sizeof(stats_packet);
    auto player_list = SinglyLinkedList{rf::player_list};
    for (auto& current_player : player_list) {
        auto& player_stats = *static_cast<PlayerStatsNew*>(current_player.stats);
        pf_player_stats_packet::player_stats out_stats{};
        out_stats.player_id = current_player.net_data->player_id;
        const pf_pure_status pure_status = pf_ac_get_pure_status(&current_player);
        out_stats.is_pure = static_cast<uint8_t>(
            get_player_additional_data(&current_player).is_browser ? pf_pure_status::rfsb : pure_status
        );
        out_stats.accuracy = static_cast<uint8_t>(player_stats.calc_accuracy() * 100.f);
        out_stats.streak_max = player_stats.max_streak;
        out_stats.streak_current = player_stats.current_streak;
        out_stats.kills = player_stats.num_kills;
        out_stats.deaths = player_stats.num_deaths;
        out_stats.team_kills = 0;
        ++stats_packet.player_count;
        if (packet_len + sizeof(out_stats) > sizeof(packet_buf)) {
            // One packet should be enough for 32 players (13 bytes * 32 players = 416 bytes)
            // Handling more data would require sending multiple packets to avoid building one big UDP datagram
            // that crosses 512 bytes limit that is usually used as maximal datagram size.
            xlog::warn("PF player_stats packet is too big: {}. Skipping some players...", packet_len);
            break;
        }
        std::memcpy(packet_buf + packet_len, &out_stats, sizeof(out_stats));
        packet_len += sizeof(out_stats);
    }

    // add reserved bytes at the end
    if (packet_len + 3 <= sizeof(packet_buf)) {
        std::memset(packet_buf + packet_len, 0, 3);
        packet_len += 3;
    }

    stats_packet.hdr.size = packet_len - sizeof(stats_packet.hdr);
    std::memcpy(packet_buf, &stats_packet, sizeof(stats_packet));
    pf_send_reliable_packet(player, packet_buf, packet_len);
}

static void process_pf_player_stats_packet(const void* data, size_t len, [[ maybe_unused ]] const rf::NetAddr& addr)
{
    // Receive: client <- server
    if (rf::is_server) {
        return;
    }

    pf_player_stats_packet stats_packet{};
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

    xlog::trace("PF player_stats packet (count {})", stats_packet.player_count);

    const std::byte* player_stats_ptr = static_cast<const std::byte*>(data) + sizeof(stats_packet);
    for (int i = 0; i < stats_packet.player_count; ++i) {
        pf_player_stats_packet::player_stats in_stats{};
        std::memcpy(&in_stats, player_stats_ptr, sizeof(in_stats));
        player_stats_ptr += sizeof(in_stats);
        auto* player = rf::multi_find_player_by_id(in_stats.player_id);
        if (player) {
            auto& stats = *static_cast<PlayerStatsNew*>(player->stats);
            if (in_stats.is_pure <= static_cast<uint8_t>(pf_pure_status::_last_variant)) {
                get_player_additional_data(player).received_ac_status
                    = std::optional{static_cast<pf_pure_status>(in_stats.is_pure)};
            }
            stats.received_accuracy = std::optional{in_stats.accuracy};
            stats.max_streak = in_stats.streak_max;
            stats.current_streak = in_stats.streak_current;
            stats.num_kills = in_stats.kills;
            stats.num_deaths = in_stats.deaths;
        }
        else {
            xlog::warn("PF player_stats packet: player {} not found", in_stats.player_id);
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

    pf_players_packet players_packet{};
    players_packet.hdr.type = static_cast<uint8_t>(pf_packet_type::players);
    players_packet.version = 1;
    players_packet.show_ip = 0;

    std::byte packet_buf[rf::max_packet_size];
    int packet_len = sizeof(players_packet);
    auto player_list = SinglyLinkedList(rf::player_list);
    for (auto& player : player_list) {
        int name_len = player.name.size();
        if (packet_len + name_len + 1 > static_cast<int>(sizeof(packet_buf))) {
            xlog::warn("PF players packet is too big! Skipping some players...");
            break;
        }
        std::memcpy(packet_buf + packet_len, player.name.c_str(), name_len + 1);
        packet_len += name_len + 1;
    }

    players_packet.hdr.size = packet_len - sizeof(players_packet.hdr);
    std::memcpy(packet_buf, &players_packet, sizeof(players_packet));
    rf::net_send(addr, packet_buf, packet_len);
}

bool pf_process_packet(const void* data, int len, const rf::NetAddr& addr, rf::Player* player)
{
    if (pf_ac_process_packet(data, len, addr, player)) {
        return true;
    }

    rf_packet_header header{};
    if (len < static_cast<int>(sizeof(header))) {
        return false;
    }

    std::memcpy(&header, data, sizeof(header));
    auto packet_type = static_cast<pf_packet_type>(header.type);

    switch (packet_type)
    {
        case pf_packet_type::announce_player:
            process_pf_player_announce_packet(data, len, addr);
            break;

        case pf_packet_type::player_stats:
            process_pf_player_stats_packet(data, len, addr);
            break;

        default:
            // ignore
            return false;
    }
    return true;
}

bool pf_process_raw_unreliable_packet(const void* data, int len, const rf::NetAddr& addr)
{
    rf_packet_header header{};
    if (len < static_cast<int>(sizeof(header))) {
        return false;
    }

    std::memcpy(&header, data, sizeof(header));
    auto packet_type = static_cast<pf_packet_type>(header.type);

    if (packet_type == pf_packet_type::players_request) {
        // Note: this packet can be sent by clients that do not join the server (e.g. online server browser)
        // Because of that it must be handled here and not in pf_process_packet
        process_pf_players_request_packet(data, len, addr);
        return true;
    }
    return false;
}

void pf_player_init([[ maybe_unused ]] rf::Player* player)
{
    assert(rf::is_server);
    pf_ac_init_player(player);
}

void pf_player_level_load(rf::Player* player)
{
    assert(rf::is_server);
    pf_ac_verify_player(player);
}

void pf_player_verified(rf::Player* player, pf_pure_status pure_status)
{
    assert(rf::is_server);
    send_pf_announce_player_packet(player, pure_status);
    send_pf_player_stats_packet(player);
}

bool pf_is_player_verified(rf::Player* player)
{
    pf_pure_status status = pf_ac_get_pure_status(player);
    return status != pf_pure_status::none;
}

int pf_get_player_ac_level(rf::Player* player)
{
    pf_pure_status status = pf_ac_get_pure_status(player);
    switch (status) {
        case pf_pure_status::blue: return 2;
        case pf_pure_status::gold: return 3;
        case pf_pure_status::fail: return 1;
        default: return 0;
    }
}
