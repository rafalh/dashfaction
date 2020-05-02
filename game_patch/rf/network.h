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
        int field_c;
        int connection_id;
        uint8_t player_id;
        uint8_t unused[3];
        int field_18;
        NwStats stats;
        int ping;
        float field_5b0;
        char packet_buf[512];
        int cb_packet_buf;
        char out_reliable_buf[512];
        int cb_out_reliable_buf;
        int connection_speed;
        int field_9c0;
        Timer field_9c4;
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

    enum MpGameMode {
        GM_DM = 0,
        GM_CTF = 1,
        GM_TEAMDM = 2,
    };

    enum MpGameOptions
    {
        GO_DEBUG_SCOREBOARD = 0x1,
        GO_LEVEL_LOADED = 0x2,
        GO_CHANGING_LEVEL = 0x4,
        GO_RANDOM_MAP_ROTATION = 0x8,
        GO_WEAPON_STAY = 0x10,
        GO_FORCE_RESPAWN = 0x20,
        GO_FALL_DAMAGE = 0x80,
        GO_REAL_FALL_DAMAGE = 0x100,
        GO_TEAM_DAMAGE = 0x240,
        GO_NOT_LAN_ONLY = 0x400,
        GO_BALANCE_TEAMS = 0x2000,
    };

    static auto& MpGetGameMode = AddrAsRef<MpGameMode()>(0x00470770);
    static auto& GetPlayersCount = AddrAsRef<unsigned()>(0x00484830);
    static auto& CtfGetRedScore = AddrAsRef<uint8_t()>(0x00475020);
    static auto& CtfGetBlueScore = AddrAsRef<uint8_t()>(0x00475030);
    static auto& TdmGetRedScore = AddrAsRef<uint8_t()>(0x004828F0);
    static auto& TdmGetBlueScore = AddrAsRef<uint8_t()>(0x00482900);
    static auto& CtfGetRedFlagPlayer = AddrAsRef<Player*()>(0x00474E60);
    static auto& CtfGetBlueFlagPlayer = AddrAsRef<Player*()>(0x00474E70);
    static auto& KickPlayer = AddrAsRef<void(Player *player)>(0x0047BF00);
    static auto& BanIp = AddrAsRef<void(const NwAddr& addr)>(0x0046D0F0);
    static auto& MultiSetRequestedWeapon = AddrAsRef<void(int weapon_cls_id)>(0x0047FCA0);
    static auto& MultiChangeLevel = AddrAsRef<void(const char* filename)>(0x0047BF50);
    static auto& PingPlayer = AddrAsRef<void(Player*)>(0x00484D00);
    static auto& SendEntityCreatePacket = AddrAsRef<void(EntityObj *entity, Player* player)>(0x00475160);
    static auto& NwInitSocket = AddrAsRef<void(unsigned short port)>(0x00528F10);

    static auto& nw_sock = AddrAsRef<int>(0x005A660C);
    static auto& nw_port = AddrAsRef<unsigned short>(0x01B587D4);
    static auto& serv_addr = AddrAsRef<NwAddr>(0x0064EC5C);
    static auto& serv_name = AddrAsRef<String>(0x0064EC28);
    static auto& is_net_game = AddrAsRef<uint8_t>(0x0064ECB9);
    static auto& is_local_net_game = AddrAsRef<uint8_t>(0x0064ECBA);
    static auto& is_dedicated_server = AddrAsRef<uint8_t>(0x0064ECBB);
    static auto& game_options = AddrAsRef<uint32_t>(0x0064EC40);
    static auto& level_rotation_idx = AddrAsRef<int>(0x0064EC64);
    static auto& server_level_list = AddrAsRef<DynamicArray<String>>(0x0064EC68);
}
