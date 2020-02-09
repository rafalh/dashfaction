#pragma once

// Forward delcarations
namespace rf
{
    class Player;
    class NwAddr;
}

void ProcessPfPacket(const void* data, size_t len, const rf::NwAddr& addr, const rf::Player* player);
