#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    // Forward declarations
    struct Player;
    struct Entity;

    // nw/psnet

    struct NetAddr
    {
        uint32_t ip_addr;
        uint16_t port;

        bool operator==(const NetAddr &other) const
        {
            return ip_addr == other.ip_addr && port == other.port;
        }

        bool operator!=(const NetAddr &other) const
        {
            return !(*this == other);
        }
    };
    static_assert(sizeof(NetAddr) == 0x8);

    static auto& net_init_socket = addr_as_ref<void(unsigned short port)>(0x00528F10);
    static auto& net_addr_to_string = addr_as_ref<void(char *buf, int buf_size, const NetAddr& addr)>(0x00529FE0);
    static auto& net_send = addr_as_ref<void(const NetAddr &addr, const void *packet, int len)>(0x0052A080);
    static auto& net_is_same = addr_as_ref<int(const NetAddr &addr1, const NetAddr &addr2, bool check_port)>(0x0052A930);

    static auto& net_udp_socket = addr_as_ref<int>(0x005A660C);
    static auto& net_port = addr_as_ref<unsigned short>(0x01B587D4);

    // multi

    struct MultiIoStats
    {
        int total_bytes_sent;
        int total_bytes_recvd;
        int bytes_sent_per_frame[30];
        int bytes_recvd_per_frame[30];
        int frame_slot_index;
        int bytes_sent_per_second[30];
        int bytes_recvd_per_second[30];
        int time_slot_index;
        TimestampRealtime packet_loss_update_timestamp;
        int obj_update_packets_sent;
        int obj_update_packets_recvd;
        int obj_update_repeated;
        int obj_update_too_slow;
        int obj_update_too_fast;
        int bytes_sent_per_type[55];
        int bytes_recvd_per_type[55];
        int packets_sent_per_type[55];
        int packets_recvd_per_type[55];
        TimestampRealtime time_slot_increment_timestamp;
        int ping_array[2];
        int current_ping_idx;
        TimestampRealtime send_ping_packet_timestamp;
        int last_ping_time;
    };
    static_assert(sizeof(MultiIoStats) == 0x590);

    struct PlayerNetData
    {
        NetAddr addr;
        int flags;
        int state;
        uint reliable_socket;
        ubyte player_id;
        int join_time_ms;
        MultiIoStats stats;
        int ping;
        float obj_update_packet_loss;
        ubyte unreliable_buffer[512];
        int unreliable_buffer_size;
        ubyte reliable_buffer[512];
        int reliable_buffer_size;
        int max_update_rate;
        int obj_update_interval;
        Timestamp obj_update_timestamp;
    };
    static_assert(sizeof(PlayerNetData) == 0x9C8);

    enum NetGameType {
        NG_TYPE_DM = 0,
        NG_TYPE_CTF = 1,
        NG_TYPE_TEAMDM = 2,
    };

    enum NetGameFlags
    {
        NG_FLAG_DEBUG_SCOREBOARD = 0x1,
        NG_FLAG_LEVEL_LOADED = 0x2,
        NG_FLAG_CHANGING_LEVEL = 0x4,
        NG_FLAG_RANDOM_MAP_ROTATION = 0x8,
        NG_FLAG_WEAPON_STAY = 0x10,
        NG_FLAG_FORCE_RESPAWN = 0x20,
        NG_FLAG_FALL_DAMAGE = 0x80,
        NG_FLAG_REAL_FALL_DAMAGE = 0x100,
        NG_FLAG_TEAM_DAMAGE = 0x240,
        NG_FLAG_NOT_LAN_ONLY = 0x400,
        NG_FLAG_BALANCE_TEAMS = 0x2000,
    };

    struct NetGameInfo
    {
        String name;
        String password;
        int field_10;
        NetGameType type;
        int flags;
        int max_players;
        int current_total_kills;
        float max_time_seconds;
        int max_kills;
        int geomod_limit;
        int max_captures;
        NetAddr server_addr;
        int current_level_index;
        VArray<String> levels;
    };

    enum class ChatMsgColor
    {
        red_white = 0,
        blue_white = 1,
        red_red = 2,
        blue_blue = 3,
        white_white = 4,
        default_ = 5,
    };

    struct BanlistEntry
    {
        char ip_addr[24];
        BanlistEntry* next;
        BanlistEntry* prev;
    };

    static auto& multi_get_game_type = addr_as_ref<NetGameType()>(0x00470770);
    static auto& multi_io_send = addr_as_ref<void(Player *player, const void *packet, int len)>(0x00479370);
    static auto& multi_io_send_reliable =
        addr_as_ref<void(Player *player, const uint8_t *data, int len, int a4)>(0x00479480);
    static auto& multi_io_send_reliable_to_all =
        addr_as_ref<void(const uint8_t *data, int len, int a4)>(0x004795A0);
    static auto& multi_find_player_by_addr = addr_as_ref<Player*(const NetAddr& addr)>(0x00484850);
    static auto& multi_find_player_by_id = addr_as_ref<Player*(uint8_t id)>(0x00484890);
    static auto& multi_ctf_get_red_team_score = addr_as_ref<uint8_t()>(0x00475020);
    static auto& multi_ctf_get_blue_team_score = addr_as_ref<uint8_t()>(0x00475030);
    static auto& multi_ctf_get_red_flag_player = addr_as_ref<Player*()>(0x00474E60);
    static auto& multi_ctf_get_blue_flag_player = addr_as_ref<Player*()>(0x00474E70);
    static auto& multi_ctf_is_red_flag_in_base = addr_as_ref<bool()>(0x00474E80);
    static auto& multi_ctf_is_blue_flag_in_base = addr_as_ref<bool()>(0x00474EA0);
    static auto& multi_tdm_get_red_team_score = addr_as_ref<uint8_t()>(0x004828F0);
    static auto& multi_tdm_get_blue_team_score = addr_as_ref<uint8_t()>(0x00482900);
    static auto& multi_num_players = addr_as_ref<int()>(0x00484830);
    static auto& multi_kick_player = addr_as_ref<void(Player *player)>(0x0047BF00);
    static auto& multi_ban_ip = addr_as_ref<void(const NetAddr& addr)>(0x0046D0F0);
    static auto& multi_set_next_weapon = addr_as_ref<void(int weapon_type)>(0x0047FCA0);
    static auto& multi_change_level = addr_as_ref<void(const char* filename)>(0x0047BF50);
    static auto& multi_ping_player = addr_as_ref<void(Player*)>(0x00484D00);
    static auto& send_entity_create_packet = addr_as_ref<void(Entity *entity, Player* player)>(0x00475160);
    static auto& send_entity_create_packet_to_all = addr_as_ref<void(Entity *entity)>(0x00475110);
    static auto& multi_find_character = addr_as_ref<int(const char *name)>(0x00476270);
    static auto& multi_chat_print = addr_as_ref<void(String::Pod text, ChatMsgColor color, String::Pod prefix)>(0x004785A0);
    static auto& multi_chat_say = addr_as_ref<void(const char *msg, bool is_team_msg)>(0x00444150);

    static auto& netgame = addr_as_ref<NetGameInfo>(0x0064EC28);
    static auto& is_multi = addr_as_ref<bool>(0x0064ECB9);
    static auto& is_server = addr_as_ref<bool>(0x0064ECBA);
    static auto& is_dedicated_server = addr_as_ref<bool>(0x0064ECBB);
    static auto& banlist_first_entry = addr_as_ref<BanlistEntry*>(0x0064EC20);
    static auto& banlist_last_entry = addr_as_ref<BanlistEntry*>(0x0064EC24);
    static auto& banlist_null_entry = addr_as_ref<BanlistEntry>(0x0064EC08);

    constexpr int multi_max_player_id = 256;
}
