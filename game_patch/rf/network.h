#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    // Forward declarations
    struct Player;
    struct Entity;

    // nw/psnet

    struct NwAddr
    {
        uint32_t ip_addr;
        uint16_t port;

        bool operator==(const NwAddr &other) const
        {
            return ip_addr == other.ip_addr && port == other.port;
        }

        bool operator!=(const NwAddr &other) const
        {
            return !(*this == other);
        }
    };
    static_assert(sizeof(NwAddr) == 0x8);

    static auto& NwInitSocket = AddrAsRef<void(unsigned short port)>(0x00528F10);
    static auto& NwAddrToString = AddrAsRef<void(char *dest, int cb_dest, const NwAddr& addr)>(0x00529FE0);
    static auto& NwSend = AddrAsRef<void(const NwAddr &addr, const void *packet, int len)>(0x0052A080);
    static auto& NwIsSame = AddrAsRef<int(const NwAddr &addr1, const NwAddr &addr2, bool check_port)>(0x0052A930);

    static auto& nw_sock = AddrAsRef<int>(0x005A660C);
    static auto& nw_port = AddrAsRef<unsigned short>(0x01B587D4);

    // multi

    struct MultiIoStats
    {
        int num_total_sent_bytes;
        int num_total_recv_bytes;
        int num_sent_bytes_unk_idx_f8[30];
        int num_recv_bytes_unk_idx_f8[30];
        int unk_idx_f8;
        int num_sent_bytes_unk_idx1_ec[30];
        int num_recv_bytes_unk_idx1_ec[30];
        int unk_idx1_ec;
        TimestampRealtime packet_loss_update_timestamp;
        int num_obj_update_packets_sent;
        int num_obj_update_packets_recv;
        int field_1fc[3];
        int num_sent_bytes_per_packet[55];
        int num_recv_bytes_per_packet[55];
        int num_packets_sent[55];
        int num_packets_recv[55];
        TimestampRealtime field_578;
        int ping_array[2];
        int current_ping_idx;
        TimestampRealtime send_ping_packet_timestamp;
        int last_ping_time;
    };
    static_assert(sizeof(MultiIoStats) == 0x590);

    struct PlayerNetData
    {
        NwAddr addr;
        int flags;
        int join_state;
        int sock_id;
        uint8_t player_id;
        int join_time_ms;
        MultiIoStats stats;
        int ping;
        float obj_update_packet_loss;
        char packet_buf[512];
        int packet_buf_len;
        char out_reliable_buf[512];
        int out_reliable_buf_len;
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
        NwAddr server_addr;
        int current_level_index;
        VArray<String> levels;
    };

    static auto& MultiGetGameType = AddrAsRef<NetGameType()>(0x00470770);
    static auto& MultiIoSend = AddrAsRef<void(Player *player, const void *packet, int len)>(0x00479370);
    static auto& MultiIoSendReliable =
        AddrAsRef<void(Player *player, const uint8_t *data, int len, int a4)>(0x00479480);
    static auto& MultiIoSendReliableToAll =
        AddrAsRef<void(const uint8_t *data, int len, int a4)>(0x004795A0);
    static auto& MultiFindPlayerByAddr = AddrAsRef<Player*(const NwAddr& addr)>(0x00484850);
    static auto& MultiFindPlayerById = AddrAsRef<Player*(uint8_t id)>(0x00484890);
    static auto& MultiCtfGetRedTeamScore = AddrAsRef<uint8_t()>(0x00475020);
    static auto& MultiCtfGetBlueTeamScore = AddrAsRef<uint8_t()>(0x00475030);
    static auto& MultiCtfGetRedFlagPlayer = AddrAsRef<Player*()>(0x00474E60);
    static auto& MultiCtfGetBlueFlagPlayer = AddrAsRef<Player*()>(0x00474E70);
    static auto& MultiCtfIsRedFlagInBase = AddrAsRef<bool()>(0x00474E80);
    static auto& MultiCtfIsBlueFlagInBase = AddrAsRef<bool()>(0x00474EA0);
    static auto& MultiTdmGetRedTeamScore = AddrAsRef<uint8_t()>(0x004828F0);
    static auto& MultiTdmGetBlueTeamScore = AddrAsRef<uint8_t()>(0x00482900);
    static auto& MultiNumPlayers = AddrAsRef<int()>(0x00484830);
    static auto& KickPlayer = AddrAsRef<void(Player *player)>(0x0047BF00);
    static auto& BanIp = AddrAsRef<void(const NwAddr& addr)>(0x0046D0F0);
    static auto& MultiSetRequestedWeapon = AddrAsRef<void(int weapon_type)>(0x0047FCA0);
    static auto& MultiChangeLevel = AddrAsRef<void(const char* filename)>(0x0047BF50);
    static auto& PingPlayer = AddrAsRef<void(Player*)>(0x00484D00);
    static auto& SendEntityCreatePacket = AddrAsRef<void(Entity *entity, Player* player)>(0x00475160);
    static auto& SendEntityCreatePacketToAll = AddrAsRef<void(Entity *entity)>(0x00475110);
    static auto& MultiFindCharacter = AddrAsRef<int(const char *name)>(0x00476270);

    static auto& netgame = AddrAsRef<NetGameInfo>(0x0064EC28);
    static auto& is_multi = AddrAsRef<bool>(0x0064ECB9);
    static auto& is_server = AddrAsRef<bool>(0x0064ECBA);
    static auto& is_dedicated_server = AddrAsRef<bool>(0x0064ECBB);
}
