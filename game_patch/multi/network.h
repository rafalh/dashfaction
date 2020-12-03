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

void network_init();
void send_chat_line_packet(const char* msg, rf::Player* target, rf::Player* sender = nullptr, bool is_team_msg = false);
const std::optional<DashFactionServerInfo>& get_df_server_info();
