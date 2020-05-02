#pragma once

#include <optional>

struct DashFactionServerInfo
{
    uint8_t version_major = 0;
    uint8_t version_minor = 0;
    bool saving_enabled = false;
};

namespace rf
{
    struct Player;
}

void NetworkInit();
void SendChatLinePacket(const char* msg, rf::Player* target, rf::Player* sender = nullptr, bool is_team_msg = false);
const std::optional<DashFactionServerInfo>& GetDashFactionServerInfo();
