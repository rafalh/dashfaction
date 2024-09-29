#pragma once

#include <optional>
#include <deque>
#include <utility>
#include "server_internal.h"
#include "../rf/player/player.h"
#include "../rf/os/timer.h"

struct PlayerStatsNew : rf::PlayerLevelStats
{
    unsigned short num_kills;
    unsigned short num_deaths;
    unsigned short current_streak;
    unsigned short max_streak;
    float num_shots_hit;
    float num_shots_fired;
    float damage_received;
    float damage_given;
    std::deque<std::pair<int, float>> damage_log; // track damage events during current life

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
        damage_log.clear();
    }

    void add_shots_hit(float add)
    {
        // Weapons with multiple projectiles (shotgun) use fractional values
        num_shots_hit += add;
    }

    void add_shots_fired(float add)
    {
        // Weapons with multiple projectiles (shotgun) use fractional values
        num_shots_fired += add;
    }

    void add_damage_received(float damage)
    {
        damage_received += damage;
    }

    void add_damage_given(float damage)
    {
        damage_given += damage;
        update_damage_log(damage);
    }

    [[nodiscard]] float calc_accuracy() const
    {
        if (num_shots_fired > 0) {
            return num_shots_hit / num_shots_fired;
        }
        return 0;
    }

    void clear()
    {
        num_kills = 0;
        num_deaths = 0;
        num_shots_hit = 0.0f;
        num_shots_fired = 0.0f;
        current_streak = 0;
        max_streak = 0;
        damage_received = 0;
        damage_given = 0;
        damage_log.clear();
    }

    // track damage events during current life
    void update_damage_log(float damage)
    {
        int current_time = rf::timer_get(1000);
        damage_log.emplace_back(current_time, damage);

        // Remove entries older than 20 seconds
        /* while (!damage_log.empty() && (current_time - damage_log.front().first) > 20000) {
            damage_log.pop_front();
        }*/
    }

    // return total damage for the past X ms
    [[nodiscard]] float get_recent_damage() const
    {
        float total_damage = 0.0f;
        int current_time = rf::timer_get(1000);

        for (const auto& entry : damage_log) {
            if ((current_time - entry.first) <= g_additional_server_config.critical_hits.dynamic_history_ms) {
                total_damage += entry.second;
            }
        }
        return total_damage;
    }
};

struct DashFactionServerInfo
{
    uint8_t version_major = 0;
    uint8_t version_minor = 0;
    bool saving_enabled = false;
    std::optional<float> max_fov;
};

void multi_level_download_update();
void multi_do_patch();
void multi_after_full_game_init();
void multi_init_player(rf::Player* player);
void send_chat_line_packet(const char* msg, rf::Player* target, rf::Player* sender = nullptr, bool is_team_msg = false);
const std::optional<DashFactionServerInfo>& get_df_server_info();
void multi_level_download_do_frame();
void multi_level_download_abort();
void multi_ban_apply_patch();
std::optional<std::string> multi_ban_unban_last();
