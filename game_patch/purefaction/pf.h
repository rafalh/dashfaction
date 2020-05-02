#pragma once

// Forward delcarations
namespace rf
{
    struct Player;
    struct NwAddr;
}

void ProcessPfPacket(const void* data, size_t len, const rf::NwAddr& addr, const rf::Player* player);
