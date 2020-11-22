#pragma once

// Forward delcarations
namespace rf
{
    struct Player;
    struct NwAddr;
}

void ProcessPfPacket(const void* data, int len, const rf::NwAddr& addr, const rf::Player* player);
