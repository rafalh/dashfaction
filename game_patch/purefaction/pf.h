#pragma once

// Forward delcarations
namespace rf
{
    struct Player;
    struct NetAddr;
}

void process_pf_packet(const void* data, int len, const rf::NetAddr& addr, rf::Player* player);
void pf_process_new_player(rf::Player* player);
