#pragma once

#include <string_view>
#include <string>
#include <set>
#include <map>
#include <optional>

// Forward declarations
namespace rf
{
    struct Player;
}

struct VoteConfig
{
    bool enabled = false;
    // int min_voters = 1;
    // int min_percentage = 50;
    int time_limit_seconds = 60;
};

struct HitSoundsConfig
{
    bool enabled = true;
    int sound_id = 29;
    int rate_limit = 10;
};

struct ServerAdditionalConfig
{
    VoteConfig vote_kick;
    VoteConfig vote_level;
    VoteConfig vote_extend;
    VoteConfig vote_restart;
    VoteConfig vote_next;
    VoteConfig vote_previous;
    VoteConfig vote_match;
    int spawn_protection_duration_ms = 1500;
    std::optional<float> spawn_life;
    std::optional<float> spawn_armor;
    HitSoundsConfig hit_sounds;
    std::map<std::string, std::string> item_replacements;
    std::string default_player_weapon;
    std::optional<int> default_player_weapon_ammo;
    bool require_client_mod = true;
    float player_damage_modifier = 1.0f;
    bool saving_enabled = false;
    bool upnp_enabled = true;
    std::optional<int> force_player_character;
    std::optional<float> max_fov;
    int anticheat_level = 0;
    bool stats_message_enabled = true;
    std::string welcome_message;
    bool weapon_items_give_full_ammo = false;
    float kill_reward_health = 0.0f;
    float kill_reward_armor = 0.0f;
    float kill_reward_effective_health = 0.0f;
    bool kill_reward_health_super = false;
    bool kill_reward_armor_super = false;
};

struct MatchInfo
{
    std::time_t last_match_reminder_time = 0;
    bool pre_match_queued = false;
    bool pre_match_active = false;
    std::time_t pre_match_start_time = 0;
    float time_limit_on_pre_match_start = 0.0f;
    std::time_t last_ready_reminder_time = 0;
    bool everyone_ready = false;
    bool match_active = false;
    int team_size = -1;
    std::set<rf::Player*> ready_players_red;
    std::set<rf::Player*> ready_players_blue;
    std::set<rf::Player*> active_match_players;
    std::string match_level_name;

    void reset()
    {
        last_match_reminder_time = 0;
        pre_match_queued = false;
        pre_match_active = false;
        pre_match_start_time = 0;
        time_limit_on_pre_match_start = 0.0f;
        last_ready_reminder_time = 0;
        everyone_ready = false;
        match_active = false;
        team_size = -1;
        ready_players_red.clear();
        ready_players_blue.clear();
        active_match_players.clear();
        match_level_name.clear();
    }
};

extern ServerAdditionalConfig g_additional_server_config;
extern std::string g_prev_level;
extern MatchInfo g_match_info;



void cleanup_win32_server_console();
void handle_vote_command(std::string_view vote_name, std::string_view vote_arg, rf::Player* sender);
std::set<rf::Player*> get_current_player_list(bool include_browsers);
std::pair<bool, std::string> is_level_name_valid(std::string_view level_name_input);
void handle_ready_command(rf::Player* sender);
void handle_unready_command(rf::Player* sender);
bool is_player_in_match(rf::Player* player);
bool is_player_ready(rf::Player* player);
void update_pre_match_powerups(rf::Player* player);
void start_match();
void cancel_match();
void start_pre_match();
void server_vote_do_frame();
void init_server_commands();
void extend_round_time(int minutes);
void restart_current_level();
void load_next_level();
void load_prev_level();
void server_vote_on_limbo_state_enter();
void process_delayed_kicks();
const ServerAdditionalConfig& server_get_df_config();
