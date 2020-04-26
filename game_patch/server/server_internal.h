#pragma once

#include <string_view>
#include <string>
#include <map>
#include <optional>

// Forward declarations
namespace rf
{
    class Player;
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
    int spawn_protection_duration_ms = 1500;
    HitSoundsConfig hit_sounds;
    std::map<std::string, std::string> item_replacements;
    std::string default_player_weapon;
    std::optional<int> default_player_weapon_ammo;
    bool require_client_mod = true;
    float player_damage_modifier = 1.0f;
    bool saving_enabled = false;
};

extern ServerAdditionalConfig g_additional_server_config;
extern std::string g_prev_level;

void InitWin32ServerConsole();
void CleanupWin32ServerConsole();
void HandleVoteCommand(std::string_view vote_name, std::string_view vote_arg, rf::Player* sender);
void ServerVoteDoFrame();
void InitLazyban();
void InitServerCommands();
void ExtendRoundTime(int minutes);
void RestartCurrentLevel();
void LoadNextLevel();
void LoadPrevLevel();
void ServerVoteOnLimboStateEnter();
