#pragma once

// Forward delcarations
namespace rf
{
    struct Player;
    struct NetAddr;
}

void process_pf_packet(const void* data, int len, const rf::NetAddr& addr, const rf::Player* player);
