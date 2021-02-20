#pragma once

#include <optional>
#include "../rf/player.h"

struct PlayerStatsNew : rf::PlayerLevelStats
{
    unsigned short num_kills;
    unsigned short num_deaths;
};

struct DashFactionServerInfo
{
    uint8_t version_major = 0;
    uint8_t version_minor = 0;
    bool saving_enabled = false;
    std::optional<float> max_fov;
};

void multi_render_level_download_progress();
void multi_do_patch();
void multi_after_full_game_init();
void multi_init_player(rf::Player* player);
void send_chat_line_packet(const char* msg, rf::Player* target, rf::Player* sender = nullptr, bool is_team_msg = false);
const std::optional<DashFactionServerInfo>& get_df_server_info();
