#pragma once

#include <string_view>
#include "../rf.h"

struct VoteConfig
{
    bool enabled = false;
    int min_voters = 1;
    int min_percentage = 50;
    int time_limit_seconds = 60;
};

struct ServerAdditionalConfig
{
    VoteConfig vote_kick;
    VoteConfig vote_level;
    VoteConfig vote_extend;
    int spawn_protection_duration_ms = 1500;
};

extern ServerAdditionalConfig g_additional_server_config;

void SendChatLinePacket(const char* msg, rf::Player* target, rf::Player* sender = nullptr, bool is_team_msg = false);
void InitWin32ServerConsole();
void CleanupWin32ServerConsole();
void HandleVoteCommand(std::string_view vote_name, std::string_view vote_arg, rf::Player* sender);
void InitLazyban();
