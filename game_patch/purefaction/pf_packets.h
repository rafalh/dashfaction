#pragma once

#include <cstdint>

#pragma pack(push, 1)

enum class pf_packet_type : uint8_t
{
    players_request = 0x3A, // no data, without game join
    server_hash = 0x3B,
    client_hash = 0x3C,
    request_cheat_check = 0x3D,
    client_cheat_check = 0x3E,
    player_stats = 0x2A,
    announce_player = 0x40,
    players = 0xA1,
};

enum class pf_pure_status : uint8_t
{
    none = 0,
    blue = 1,
    gold = 2,
    fail = 3,
    old_pure = 4,
    rfsb = 5,
    _last_variant = rfsb,
};

struct rf_packet_header
{
    uint8_t type; // see pf_packet_type
    uint16_t size; // size of data without header (PF usually includes header size here)
};

struct pf_player_stats_packet
{
    rf_packet_header hdr; // player_stats
    uint8_t version; // current version is 2
    uint8_t player_count;

    struct player_stats
    {
        uint8_t player_id;
        uint8_t is_pure;
        uint8_t accuracy;
        uint16_t streak_max;
        uint16_t streak_current;
        uint16_t kills;
        uint16_t deaths;
        uint16_t team_kills;
    };
#ifdef PSEUDOCODE
    player_stats players[];
    uint8_t reserved[3]; // PF bug workaround (PF expects size field to include header size)
#endif
};

constexpr uint8_t pf_player_stats_packet_version = 2;

struct pf_player_announce_packet
{
    rf_packet_header hdr; // announce_player
    uint8_t version; // current version is 2
    uint8_t player_id;
    uint8_t is_pure; // 0 non-pure, 1 pure (public), 2 pure (match), 3 check failed, 4 unused, 5 rfsb
    uint8_t reserved[3]; // PF bug workaround (PF expects size field to include header size)
};

constexpr uint8_t pf_announce_player_packet_version = 2;

struct pf_players_packet
{
    rf_packet_header hdr; // players
    uint8_t version; // current version is 1
    uint8_t show_ip;
#ifdef PSEUDOCODE
    struct player_info
    {
        if (show_ip) {
            uint32_t ip_addr;
        }
        char name[];
    };
    player_info players[];
    uint8_t reserved[3]; // PF bug workaround (PF expects size field to include header size)
#endif
};

constexpr uint8_t pf_players_packet_version = 1;

constexpr uint32_t pf_game_info_signature = 0xEFBEADDE;
constexpr uint16_t pf_game_info_version = 0x030DF;

// payload added to game_info packet to allow detection of Pure Faction servers
struct pf_game_info_packet_suffix {
    uint32_t signature; // 0xEFBEADDE
    uint16_t pf_version; // current is 0x030DF
};

#pragma pack(pop)
