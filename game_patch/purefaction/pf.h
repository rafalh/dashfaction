#pragma once

// Forward delcarations
namespace rf
{
    struct Player;
    struct NetAddr;
}

bool pf_process_packet(const void* data, int len, const rf::NetAddr& addr, rf::Player* player);
bool pf_process_raw_unreliable_packet(const void* data, int len, const rf::NetAddr& addr);
void pf_player_init(rf::Player* player);
void pf_player_level_load(rf::Player* player);
bool pf_is_player_verified(rf::Player* player);
