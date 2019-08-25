#pragma once

#include <string_view>
#include <string>
#include "../rf.h"

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
};

extern ServerAdditionalConfig g_additional_server_config;
extern std::string g_prev_level;

void SendChatLinePacket(const char* msg, rf::Player* target, rf::Player* sender = nullptr, bool is_team_msg = false);
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
