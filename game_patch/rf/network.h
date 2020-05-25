#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    // Forward declarations
    struct Player;
    struct EntityObj;

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

    struct NwStats
    {
        int num_total_sent_bytes;
        int num_total_recv_bytes;
        int num_sent_bytes_unk_idx_f8[30];
        int num_recv_bytes_unk_idx_f8[30];
        int unk_idx_f8;
        int num_sent_bytes_unk_idx1_ec[30];
        int num_recv_bytes_unk_idx1_ec[30];
        int unk_idx1_ec;
        TimerApp packet_loss_update_timer;
        int num_obj_update_packets_sent;
        int num_obj_update_packets_recv;
        int field_1fc[3];
        int num_sent_bytes_per_packet[55];
        int num_recv_bytes_per_packet[55];
        int num_packets_sent[55];
        int num_packets_recv[55];
        TimerApp field_578;
        int ping_array[2];
        int current_ping_idx;
        TimerApp send_ping_packet_timer;
        int last_ping_time;
    };
    static_assert(sizeof(NwStats) == 0x590);

    typedef int NwPlayerFlags;

    struct PlayerNetData
    {
        NwAddr addr;
        NwPlayerFlags flags;
        int join_state;
        int sock_id;
        uint8_t player_id;
        int join_time_ms;
        NwStats stats;
        int ping;
        float obj_update_packet_loss;
        char packet_buf[512];
        int packet_buf_len;
        char out_reliable_buf[512];
        int out_reliable_buf_len;
        int max_update_rate;
        int obj_update_interval;
        Timer obj_update_timer;
    };
    static_assert(sizeof(PlayerNetData) == 0x9C8);

    static auto& NwSendNotReliablePacket =
        AddrAsRef<void(const NwAddr &addr, const void *packet, unsigned cb_packet)>(0x0052A080);
    static auto& NwSendReliablePacket =
        AddrAsRef<void(Player *player, const uint8_t *data, unsigned int num_bytes, int a4)>(0x00479480);
    static auto& NwSendReliablePacketToAll =
        AddrAsRef<void( const uint8_t *data, unsigned int num_bytes, int a4)>(0x004795A0);
    static auto& NwAddrToStr = AddrAsRef<void(char *dest, int cb_dest, const NwAddr& addr)>(0x00529FE0);
    static auto& NwGetPlayerFromAddr = AddrAsRef<Player*(const NwAddr& addr)>(0x00484850);
    static auto& NwCompareAddr = AddrAsRef<int(const NwAddr &addr1, const NwAddr &addr2, bool check_port)>(0x0052A930);
    static auto& GetPlayerById = AddrAsRef<Player*(uint8_t id)>(0x00484890);

    /* Network Game */

    enum MultiGameType {
        MGT_DM = 0,
        MGT_CTF = 1,
        MGT_TEAMDM = 2,
    };

    enum MultiGameOptions
    {
        MGO_DEBUG_SCOREBOARD = 0x1,
        MGO_LEVEL_LOADED = 0x2,
        MGO_CHANGING_LEVEL = 0x4,
        MGO_RANDOM_MAP_ROTATION = 0x8,
        MGO_WEAPON_STAY = 0x10,
        MGO_FORCE_RESPAWN = 0x20,
        MGO_FALL_DAMAGE = 0x80,
        MGO_REAL_FALL_DAMAGE = 0x100,
        MGO_TEAM_DAMAGE = 0x240,
        MGO_NOT_LAN_ONLY = 0x400,
        MGO_BALANCE_TEAMS = 0x2000,
    };

    static auto& MultiGetGameType = AddrAsRef<MultiGameType()>(0x00470770);
    static auto& CtfGetRedTeamScore = AddrAsRef<uint8_t()>(0x00475020);
    static auto& CtfGetBlueTeamScore = AddrAsRef<uint8_t()>(0x00475030);
    static auto& CtfGetRedFlagPlayer = AddrAsRef<Player*()>(0x00474E60);
    static auto& CtfGetBlueFlagPlayer = AddrAsRef<Player*()>(0x00474E70);
    static auto& CtfIsRedFlagInBase = AddrAsRef<bool()>(0x00474E80);
    static auto& CtfIsBlueFlagInBase = AddrAsRef<bool()>(0x00474EA0);
    static auto& TdmGetRedTeamScore = AddrAsRef<uint8_t()>(0x004828F0);
    static auto& TdmGetBlueTeamScore = AddrAsRef<uint8_t()>(0x00482900);
    static auto& GetPlayersCount = AddrAsRef<unsigned()>(0x00484830);
    static auto& KickPlayer = AddrAsRef<void(Player *player)>(0x0047BF00);
    static auto& BanIp = AddrAsRef<void(const NwAddr& addr)>(0x0046D0F0);
    static auto& MultiSetRequestedWeapon = AddrAsRef<void(int weapon_cls_id)>(0x0047FCA0);
    static auto& MultiChangeLevel = AddrAsRef<void(const char* filename)>(0x0047BF50);
    static auto& PingPlayer = AddrAsRef<void(Player*)>(0x00484D00);
    static auto& SendEntityCreatePacket = AddrAsRef<void(EntityObj *entity, Player* player)>(0x00475160);
    static auto& SendEntityCreatePacketToAll = AddrAsRef<void(EntityObj *entity)>(0x00475110);
    static auto& NwInitSocket = AddrAsRef<void(unsigned short port)>(0x00528F10);

    static auto& nw_sock = AddrAsRef<int>(0x005A660C);
    static auto& nw_port = AddrAsRef<unsigned short>(0x01B587D4);
    static auto& serv_name = AddrAsRef<String>(0x0064EC28);
    static auto& multi_game_options = AddrAsRef<uint32_t>(0x0064EC40);
    static auto& serv_addr = AddrAsRef<NwAddr>(0x0064EC5C);
    static auto& level_rotation_idx = AddrAsRef<int>(0x0064EC64);
    static auto& server_levels = AddrAsRef<DynamicArray<String>>(0x0064EC68);
    static auto& is_multi = AddrAsRef<bool>(0x0064ECB9);
    static auto& is_server = AddrAsRef<bool>(0x0064ECBA);
    static auto& is_dedicated_server = AddrAsRef<bool>(0x0064ECBB);
}
