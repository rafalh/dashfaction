#pragma once

#include <optional>
#include "../rf/player/player.h"

struct PlayerStatsNew : rf::PlayerLevelStats
{
    unsigned short num_kills;
    unsigned short num_deaths;
    unsigned short current_streak;
    unsigned short max_streak;
    unsigned int num_shots_hit;
    unsigned int num_shots_fired;
    float damage_received;
    float damage_given;

    void inc_kills()
    {
        ++num_kills;
        ++current_streak;
        max_streak = std::max(max_streak, current_streak);
    }

    void inc_deaths()
    {
        ++num_deaths;
        current_streak = 0;
    }

    void inc_shots_hit()
    {
        ++num_shots_hit;
    }

    void inc_shots_fired()
    {
        ++num_shots_fired;
    }

    void add_damage_received(float damage)
    {
        damage_received += damage;
    }

    void add_damage_given(float damage)
    {
        damage_given += damage;
    }

    float calc_accuracy() const
    {
        if (num_shots_fired > 0) {
            return static_cast<float>(num_shots_hit) / num_shots_fired;
        }
        return 0;
    }

    void clear()
    {
        num_kills = 0;
        num_deaths = 0;
        num_shots_hit = 0;
        num_shots_fired = 0;
        current_streak = 0;
        max_streak = 0;
        damage_received = 0;
        damage_given = 0;
    }
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
void multi_level_download_do_frame();
void multi_level_download_abort();
